using System.IO.Ports;

namespace MeasurementMonitor;

public partial class MainForm : Form
{
    private enum PendingRead { None, Wifi, Measurement }
    private const int MinimumSettingsResponseTimeoutMs = 5000;
    private readonly SerialPort port = new();
    private readonly ProtocolFramer framer = new();
    private readonly object receiveLock = new();
    private PendingRead pendingRead;
    private long pendingReadExpires;
    private WifiSettings? savedWifi;
    private MeasurementSettings? savedMeasurement;
    private SerialSettings? savedSerial;
    private LayoutSettings? savedLayout;

    public MainForm()
    {
        InitializeComponent();

        serialPanel.OpenCloseRequested += (_, _) => TogglePort();
        serialPanel.ClearRequested += (_, _) => monitorPanel.ClearLog();
        wifiPanel.ReadRequested += (_, _) => SendRead(PendingRead.Wifi, DeviceProtocol.WifiRead());
        wifiPanel.WriteRequested += (_, value) => WriteWifi(value);
        measurementPanel.ReadRequested += (_, _) => SendRead(PendingRead.Measurement, DeviceProtocol.MeasurementRead());
        measurementPanel.WriteRequested += (_, value) => WriteMeasurement(value);
        port.DataReceived += PortDataReceived;
        FormClosing += (_, _) =>
        {
            SaveSettings(wifiPanel.CurrentSettings, measurementPanel.CurrentSettings,
                serialPanel.CurrentSettings, CurrentLayout);
            if (port.IsOpen) port.Close();
        };

        SavedAppSettings saved = AppSettingsStore.Load();
        savedWifi = saved.Wifi;
        savedMeasurement = saved.Measurement;
        savedSerial = saved.Serial;
        savedLayout = saved.Layout;
        if (savedWifi is not null) wifiPanel.Apply(savedWifi);
        if (savedMeasurement is not null) measurementPanel.Apply(savedMeasurement);
        if (savedSerial is not null) serialPanel.Apply(savedSerial);
        Shown += (_, _) => ApplySavedLayout();
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
                SaveSettings(savedWifi, savedMeasurement, serialPanel.CurrentSettings, CurrentLayout);
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
        SaveSettings(value, savedMeasurement, savedSerial, CurrentLayout);
        TrySend(() => DeviceProtocol.WifiWrite(value));
    }
    private void WriteMeasurement(MeasurementSettings value)
    {
        SaveSettings(savedWifi, value, savedSerial, CurrentLayout);
        TrySend(() => DeviceProtocol.MeasurementWrite(value));
    }
    private void SaveSettings(WifiSettings? wifi, MeasurementSettings? measurement,
        SerialSettings? serial, LayoutSettings? layout)
    {
        savedWifi = wifi;
        savedMeasurement = measurement;
        savedSerial = serial;
        savedLayout = layout;
        if (!AppSettingsStore.Save(new(wifi, measurement, serial, layout)))
            monitorPanel.AddOther("[PC 설정 저장 실패] settings.json 파일을 기록할 수 없습니다.");
    }
    private LayoutSettings CurrentLayout => new(contentSplit.SplitterDistance,
        settingsSplit.SplitterDistance);
    private void ApplySavedLayout()
    {
        if (savedLayout is null) return;
        contentSplit.SplitterDistance = ClampSplitter(contentSplit, savedLayout.SettingsHeight);
        settingsSplit.SplitterDistance = ClampSplitter(settingsSplit, savedLayout.WifiWidth);
    }
    private static int ClampSplitter(SplitContainer split, int value)
    {
        int total = split.Orientation == Orientation.Vertical ? split.ClientSize.Width : split.ClientSize.Height;
        int maximum = Math.Max(split.Panel1MinSize, total - split.Panel2MinSize - split.SplitterWidth);
        return Math.Clamp(value, split.Panel1MinSize, maximum);
    }
    private void SendRead(PendingRead kind, byte[] frame)
    {
        if (Send(frame))
        {
            pendingRead = kind;
            // 설정 응답은 command 없이 값만 오므로 일반 serial timeout보다 길게 기다립니다.
            pendingReadExpires = Environment.TickCount64 +
                Math.Max(serialPanel.Timeout, MinimumSettingsResponseTimeoutMs);
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
            SaveSettings(wifi, savedMeasurement, savedSerial, CurrentLayout);
        }
        else if (received.HasStx && DeviceProtocol.TryParseMeasurementSettings(frame, out MeasurementSettings? measurement) && measurement is not null)
        {
            pendingRead = PendingRead.None;
            measurementPanel.Apply(measurement);
            SaveSettings(savedWifi, measurement, savedSerial, CurrentLayout);
        }
        else if (received.HasStx && Environment.TickCount64 <= pendingReadExpires && pendingRead == PendingRead.Wifi &&
                 DeviceProtocol.TryParseWifiSettings(frame, out wifi, true) && wifi is not null)
        {
            pendingRead = PendingRead.None;
            wifiPanel.Apply(wifi);
            SaveSettings(wifi, savedMeasurement, savedSerial, CurrentLayout);
        }
        else if (received.HasStx && Environment.TickCount64 <= pendingReadExpires && pendingRead == PendingRead.Measurement &&
                 DeviceProtocol.TryParseMeasurementSettings(frame, out measurement, true) && measurement is not null)
        {
            pendingRead = PendingRead.None;
            measurementPanel.Apply(measurement);
            SaveSettings(savedWifi, measurement, savedSerial, CurrentLayout);
        }
        else if (!received.HasStx) monitorPanel.AddOther(frame, false);
        else if (frame.StartsWith("WIFI_R_ALL", StringComparison.OrdinalIgnoreCase) ||
                 frame.StartsWith("MEAS_R_ALL", StringComparison.OrdinalIgnoreCase))
            monitorPanel.AddOther($"[설정 응답 형식 오류] {frame}");
        else if (DeviceProtocol.IsMeasurementData(frame)) monitorPanel.AddMeasurement(frame);
        else monitorPanel.AddOther(frame);
    }
}
