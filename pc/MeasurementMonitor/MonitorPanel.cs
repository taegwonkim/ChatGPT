namespace MeasurementMonitor;

public partial class MonitorPanel : UserControl
{
    public MonitorPanel()
    {
        InitializeComponent();
    }
    internal void ClearLog()
    {
        measurementLog.Clear();
        otherLog.Clear();
        status.Text = "STATUS: -";
        macAddress.Text = "MAC Address: -";
    }
    internal void AddFrame(string frame) => AddOther(frame);
    internal void AddMeasurement(string frame) => Append(measurementLog, frame);
    internal void AddOther(string frame, bool hasStx = true)
    {
        if (frame.StartsWith("STATUS", StringComparison.OrdinalIgnoreCase)) status.Text = frame;
        if (frame.StartsWith("MAC_", StringComparison.OrdinalIgnoreCase))
        {
            string value = frame[4..].Trim();
            macAddress.Text = value.Length == 0 ? "MAC Address: -" : $"MAC Address: {value}";
        }
        Append(otherLog, hasStx ? frame : $"[RAW] {frame}");
    }
    private void Append(RichTextBox target, string frame)
    {
        target.AppendText($"[{DateTime.Now:HH:mm:ss.fff}] {frame}{Environment.NewLine}");
        if (autoScroll.Checked) { target.SelectionStart = target.TextLength; target.ScrollToCaret(); }
    }
}
