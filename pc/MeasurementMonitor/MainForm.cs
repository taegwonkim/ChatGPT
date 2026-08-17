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
    private WifiSettings? savedWifi;
    private MeasurementSettings? savedMeasurement;

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
        wifiPanel.WriteRequested += (_, value) => WriteWifi(value);
        measurementPanel.ReadRequested += (_, _) => SendRead(PendingRead.Measurement, DeviceProtocol.MeasurementRead());
        measurementPanel.WriteRequested += (_, value) => WriteMeasurement(value);
        port.DataReceived += PortDataReceived;
        FormClosing += (_, _) =>
        {
            SaveSettings(wifiPanel.CurrentSettings, measurementPanel.CurrentSettings);
            if (port.IsOpen) port.Close();
        };

        SavedAppSettings saved = AppSettingsStore.Load();
        savedWifi = saved.Wifi;
        savedMeasurement = saved.Measurement;
        if (savedWifi is not null) wifiPanel.Apply(savedWifi);
        if (savedMeasurement is not null) measurementPanel.Apply(savedMeasurement);
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
    private void WriteWifi(WifiSettings value)
    {
        SaveSettings(value, savedMeasurement);
        TrySend(() => DeviceProtocol.WifiWrite(value));
    }
    private void WriteMeasurement(MeasurementSettings value)
    {
        SaveSettings(savedWifi, value);
        TrySend(() => DeviceProtocol.MeasurementWrite(value));
    }
    private void SaveSettings(WifiSettings? wifi, MeasurementSettings? measurement)
    {
        savedWifi = wifi;
        savedMeasurement = measurement;
        if (!AppSettingsStore.Save(new(wifi, measurement)))
            monitorPanel.AddOther("[PC 설정 저장 실패] settings.json 파일을 기록할 수 없습니다.");
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
        if (received.HasStx && DeviceProtocol.TryParseWifiSettings(frame, out WifiSettings? wifi) && wifi is not null)
        {
            pendingRead = PendingRead.None;
            wifiPanel.Apply(wifi);
            SaveSettings(wifi, savedMeasurement);
        }
        else if (received.HasStx && DeviceProtocol.TryParseMeasurementSettings(frame, out MeasurementSettings? measurement) && measurement is not null)
        {
            pendingRead = PendingRead.None;
            measurementPanel.Apply(measurement);
            SaveSettings(savedWifi, measurement);
        }
        else if (received.HasStx && Environment.TickCount64 <= pendingReadExpires && pendingRead == PendingRead.Wifi &&
                 DeviceProtocol.TryParseWifiSettings(frame, out wifi, true) && wifi is not null)
        {
            pendingRead = PendingRead.None;
            wifiPanel.Apply(wifi);
            SaveSettings(wifi, savedMeasurement);
        }
        else if (received.HasStx && Environment.TickCount64 <= pendingReadExpires && pendingRead == PendingRead.Measurement &&
                 DeviceProtocol.TryParseMeasurementSettings(frame, out measurement, true) && measurement is not null)
        {
            pendingRead = PendingRead.None;
            measurementPanel.Apply(measurement);
            SaveSettings(savedWifi, measurement);
        }
        else if (!received.HasStx) monitorPanel.AddOther(frame, false);
        else if (frame.StartsWith("WIFI_R_ALL", StringComparison.OrdinalIgnoreCase) ||
                 frame.StartsWith("MEAS_R_ALL", StringComparison.OrdinalIgnoreCase))
            monitorPanel.AddOther($"[설정 응답 형식 오류] {frame}");
        else if (DeviceProtocol.IsMeasurementData(frame)) monitorPanel.AddMeasurement(frame);
        else monitorPanel.AddOther(frame);
    }
}
