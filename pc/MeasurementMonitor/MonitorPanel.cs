namespace MeasurementMonitor;

public partial class MonitorPanel : UserControl
{
    public MonitorPanel()
    {
        InitializeComponent();
        ApplyStatusMacStyle();
    }
    internal void ApplyStatusMacStyle()
    {
        SetValueStyle(status);
        SetValueStyle(macAddress);
    }
    internal int DataSplitterDistance
    {
        get => dataSplit.SplitterDistance;
        set
        {
            int maximum = Math.Max(dataSplit.Panel1MinSize,
                dataSplit.ClientSize.Width - dataSplit.Panel2MinSize - dataSplit.SplitterWidth);
            dataSplit.SplitterDistance = Math.Clamp(value, dataSplit.Panel1MinSize, maximum);
        }
    }
    private static void SetValueStyle(Label valueLabel)
    {
        valueLabel.BackColor = Color.White;
        valueLabel.ForeColor = Color.Black;
        valueLabel.BorderStyle = BorderStyle.FixedSingle;
    }
    internal void ClearLog()
    {
        measurementLog.Clear();
        otherLog.Clear();
        status.Text = "-";
        macAddress.Text = "-";
    }
    internal void AddFrame(string frame) => AddOther(frame);
    internal void AddMeasurement(string frame) => Append(measurementLog, frame);
    internal void AddOther(string frame, bool hasStx = true)
    {
        if (frame.StartsWith("STATUS", StringComparison.OrdinalIgnoreCase))
            status.Text = ExtractValue(frame, "STATUS");
        if (frame.StartsWith("MAC_", StringComparison.OrdinalIgnoreCase))
        {
            macAddress.Text = ExtractValue(frame, "MAC");
        }
        Append(otherLog, hasStx ? frame : $"[RAW] {frame}");
    }
    private void Append(RichTextBox target, string frame)
    {
        target.AppendText($"[{DateTime.Now:HH:mm:ss.fff}] {frame}{Environment.NewLine}");
        if (autoScroll.Checked) { target.SelectionStart = target.TextLength; target.ScrollToCaret(); }
    }
    private static string ExtractValue(string frame, string prefix)
    {
        string value = frame[prefix.Length..].TrimStart('_', ',', ':', '=', ' ').Trim();
        return value.Length == 0 ? "-" : value;
    }
}
