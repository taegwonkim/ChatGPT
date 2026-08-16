using System.IO.Ports;

namespace MeasurementMonitor;

internal sealed class MainForm : Form
{
    private readonly SerialPanel serialPanel = new();
    private readonly WifiPanel wifiPanel = new();
    private readonly MeasurementPanel measurementPanel = new();
    private readonly MonitorPanel monitorPanel = new();
    private readonly SerialPort port = new();
    private readonly ProtocolFramer framer = new();
    private readonly object receiveLock = new();

    internal MainForm()
    {
        Text = "STM32 Measurement Monitor"; MinimumSize = new(900, 650); StartPosition = FormStartPosition.CenterScreen;
        var settings = new TableLayoutPanel { Dock = DockStyle.Top, Height = 245, ColumnCount = 2 };
        settings.ColumnStyles.Add(new(SizeType.Percent, 55)); settings.ColumnStyles.Add(new(SizeType.Percent, 45));
        settings.Controls.Add(wifiPanel, 0, 0); settings.Controls.Add(measurementPanel, 1, 0);
        Controls.Add(monitorPanel); Controls.Add(settings); Controls.Add(serialPanel);

        serialPanel.OpenCloseRequested += (_, _) => TogglePort();
        serialPanel.ClearRequested += (_, _) => monitorPanel.ClearLog();
        wifiPanel.ReadRequested += (_, _) => Send(DeviceProtocol.WifiRead());
        wifiPanel.WriteRequested += (_, value) => TrySend(() => DeviceProtocol.WifiWrite(value));
        measurementPanel.ReadRequested += (_, _) => Send(DeviceProtocol.MeasurementRead());
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
    private void Send(byte[] frame)
    {
        try
        {
            if (!port.IsOpen) throw new InvalidOperationException("먼저 serial port를 여십시오.");
            port.Write(frame, 0, frame.Length);
        }
        catch (Exception ex) { MessageBox.Show(ex.Message, "송신 오류", MessageBoxButtons.OK, MessageBoxIcon.Error); }
    }
    private void PortDataReceived(object? sender, SerialDataReceivedEventArgs e)
    {
        try
        {
            byte[] buffer = new byte[port.BytesToRead]; int count = port.Read(buffer, 0, buffer.Length);
            List<string> frames;
            lock (receiveLock) frames = framer.Push(buffer.AsSpan(0, count)).ToList();
            foreach (string frame in frames)
                BeginInvoke(new Action(() => HandleFrame(frame)));
        }
        catch (Exception ex) when (ex is IOException or InvalidOperationException) { }
    }

    private void HandleFrame(string frame)
    {
        if (DeviceProtocol.TryParseWifiSettings(frame, out WifiSettings? wifi) && wifi is not null)
        {
            wifiPanel.Apply(wifi);
            monitorPanel.AddFrame($"{frame.Split(',')[0]},{wifi.Ssid},********,{wifi.ServerIp},{wifi.ServerPort},{(wifi.Dhcp ? 1 : 0)},{wifi.LocalIp},{wifi.Gateway},{wifi.Netmask}");
        }
        else if (DeviceProtocol.TryParseMeasurementSettings(frame, out MeasurementSettings? measurement) && measurement is not null)
        {
            measurementPanel.Apply(measurement);
            monitorPanel.AddFrame(frame);
        }
        else monitorPanel.AddFrame(frame);
    }
}
