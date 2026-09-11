namespace MeasurementMonitor;

public partial class MonitorPanel : UserControl
{
    public MonitorPanel()
    {
        InitializeComponent();
        RestoreHeaderLayout();
        ApplyStatusMacStyle();
        dataSplit.SplitterMoved += (_, _) => DataSplitterMoved?.Invoke(this, EventArgs.Empty);
    }
    internal event EventHandler? DataSplitterMoved;
    internal void RestoreHeaderLayout()
    {
        // 이전 WYSIWYG layout 파일이 header 항목을 다른 parent나 화면 밖으로 옮겼어도
        // STATUS/MAC 영역만큼은 항상 header 안의 정해진 열에 표시합니다.
        PlaceHeaderControl(autoScroll, 0);
        PlaceHeaderControl(statusCaption, 1);
        PlaceHeaderControl(status, 2);
        PlaceHeaderControl(macAddressCaption, 4);
        PlaceHeaderControl(macAddress, 5);
        headerLayout.Visible = true;
        headerLayout.BringToFront();
    }
    private void PlaceHeaderControl(Control control, int column)
    {
        if (control.Parent != headerLayout)
            headerLayout.Controls.Add(control, column, 0);
        else
        {
            headerLayout.SetColumn(control, column);
            headerLayout.SetRow(control, 0);
        }
        control.Dock = DockStyle.None;
        control.Visible = true;
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
