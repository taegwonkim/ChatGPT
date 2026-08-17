using System.IO.Ports;

namespace MeasurementMonitor;

public class MainForm : Form
{
    private enum PendingRead { None, Wifi, Measurement }
    private readonly SerialPanel serialPanel = new();
    private readonly WifiPanel wifiPanel = new();
    private readonly MeasurementPanel measurementPanel = new();
    private readonly MonitorPanel monitorPanel = new();
    private readonly SerialPort port = new();
    private readonly ProtocolFramer framer = new();
    private readonly object receiveLock = new();
    private readonly ToolTip toolTip = new();
    private PendingRead pendingRead;
    private long pendingReadExpires;

    public MainForm()
    {
        Text = "STM32 Measurement Monitor - Visual Studio 2022"; MinimumSize = new(900, 650); StartPosition = FormStartPosition.CenterScreen;
        var settings = new TableLayoutPanel { Dock = DockStyle.Top, Height = 245, ColumnCount = 2 };
        settings.ColumnStyles.Add(new(SizeType.Percent, 55)); settings.ColumnStyles.Add(new(SizeType.Percent, 45));
        settings.Controls.Add(wifiPanel, 0, 0); settings.Controls.Add(measurementPanel, 1, 0);
        Controls.Add(monitorPanel); Controls.Add(settings); Controls.Add(serialPanel);
        toolTip.SetToolTip(serialPanel, "COM port 연결과 수신 화면 지우기");
        toolTip.SetToolTip(wifiPanel, "AP/Server/DHCP 설정 Read 및 Write");
        toolTip.SetToolTip(measurementPanel, "측정 조건 설정 Read 및 Write");
        toolTip.SetToolTip(monitorPanel, "MCU 측정값 및 STATUS frame 표시");

        serialPanel.OpenCloseRequested += (_, _) => TogglePort();
        serialPanel.ClearRequested += (_, _) => monitorPanel.ClearLog();
        wifiPanel.ReadRequested += (_, _) => SendRead(PendingRead.Wifi, DeviceProtocol.WifiRead());
        wifiPanel.WriteRequested += (_, value) => TrySend(() => DeviceProtocol.WifiWrite(value));
        measurementPanel.ReadRequested += (_, _) => SendRead(PendingRead.Measurement, DeviceProtocol.MeasurementRead());
        measurementPanel.WriteRequested += (_, value) => TrySend(() => DeviceProtocol.MeasurementWrite(value));
        port.DataReceived += PortDataReceived;
        FormClosing += (_, _) => { if (port.IsOpen) port.Close(); };
    }

    private void TogglePort()
    {
        try
        {
            if (port.IsOpen) port.Close();
            else
            {
                if (string.IsNullOrWhiteSpace(serialPanel.PortName)) throw new InvalidOperationException("COM port를 선택하십시오.");
                port.PortName = serialPanel.PortName; port.BaudRate = serialPanel.BaudRate;
                port.DataBits = 8; port.Parity = Parity.None; port.StopBits = StopBits.One;
                port.ReadTimeout = port.WriteTimeout = serialPanel.Timeout; port.Open();
            }
            serialPanel.SetOpen(port.IsOpen);
            monitorPanel.AddFrame(port.IsOpen ? $"STATUS,PORT_OPEN,{port.PortName}" : "STATUS,PORT_CLOSED");
        }
        catch (Exception ex) { MessageBox.Show(ex.Message, "Serial port 오류", MessageBoxButtons.OK, MessageBoxIcon.Error); }
    }

    private void TrySend(Func<byte[]> build)
    {
        try { Send(build()); }
        catch (Exception ex) { MessageBox.Show(ex.Message, "입력 오류", MessageBoxButtons.OK, MessageBoxIcon.Warning); }
    }
    private void SendRead(PendingRead kind, byte[] frame)
    {
        if (Send(frame))
        {
            pendingRead = kind;
            pendingReadExpires = Environment.TickCount64 + serialPanel.Timeout;
        }
    }
    private bool Send(byte[] frame)
    {
        try
        {
            if (!port.IsOpen) throw new InvalidOperationException("먼저 serial port를 여십시오.");
            port.Write(frame, 0, frame.Length);
            return true;
        }
        catch (Exception ex) { MessageBox.Show(ex.Message, "송신 오류", MessageBoxButtons.OK, MessageBoxIcon.Error); return false; }
    }
    private void PortDataReceived(object? sender, SerialDataReceivedEventArgs e)
    {
        try
        {
            byte[] buffer = new byte[port.BytesToRead]; int count = port.Read(buffer, 0, buffer.Length);
            List<ReceivedFrame> frames;
            lock (receiveLock) frames = framer.Push(buffer.AsSpan(0, count)).ToList();
            foreach (ReceivedFrame frame in frames)
                BeginInvoke(new Action(() => HandleFrame(frame)));
        }
        catch (Exception ex) when (ex is IOException or InvalidOperationException) { }
    }

    private void HandleFrame(ReceivedFrame received)
    {
        string frame = received.Payload;
        if (DeviceProtocol.TryParseWifiSettings(frame, out WifiSettings? wifi) && wifi is not null)
        {
            pendingRead = PendingRead.None;
            wifiPanel.Apply(wifi);
            monitorPanel.AddOther($"WIFI_R_ALL,{wifi.Ssid},********,{wifi.ServerIp},{wifi.ServerPort},{(wifi.Dhcp ? 1 : 0)},{wifi.LocalIp},{wifi.Gateway},{wifi.Netmask}", received.HasStx);
        }
        else if (DeviceProtocol.TryParseMeasurementSettings(frame, out MeasurementSettings? measurement) && measurement is not null)
        {
            pendingRead = PendingRead.None;
            measurementPanel.Apply(measurement);
            monitorPanel.AddOther(frame, received.HasStx);
        }
        else if (Environment.TickCount64 <= pendingReadExpires && pendingRead == PendingRead.Wifi &&
                 DeviceProtocol.TryParseWifiSettings(frame, out wifi, true) && wifi is not null)
        {
            pendingRead = PendingRead.None;
            wifiPanel.Apply(wifi);
            monitorPanel.AddOther($"WIFI_R_ALL,{wifi.Ssid},********,{wifi.ServerIp},{wifi.ServerPort},{(wifi.Dhcp ? 1 : 0)},{wifi.LocalIp},{wifi.Gateway},{wifi.Netmask}", received.HasStx);
        }
        else if (Environment.TickCount64 <= pendingReadExpires && pendingRead == PendingRead.Measurement &&
                 DeviceProtocol.TryParseMeasurementSettings(frame, out measurement, true) && measurement is not null)
        {
            pendingRead = PendingRead.None;
            measurementPanel.Apply(measurement);
            monitorPanel.AddOther($"MEAS_R_ALL,{frame}", received.HasStx);
        }
        else if (!received.HasStx) monitorPanel.AddOther(frame, false);
        else if (DeviceProtocol.IsMeasurementData(frame)) monitorPanel.AddMeasurement(frame);
        else monitorPanel.AddOther(frame);
    }
}
