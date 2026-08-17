namespace MeasurementMonitor;

public class MonitorPanel : GroupBox
{
    private readonly RichTextBox measurementLog = LogBox();
    private readonly RichTextBox otherLog = LogBox();
    private readonly CheckBox autoScroll = new() { Text = "Auto scroll", Checked = true, AutoSize = true };
    private readonly Label status = new() { Text = "STATUS: -", AutoSize = true, Padding = new Padding(5) };

    public MonitorPanel()
    {
        Text = "MCU 수신 데이터"; Dock = DockStyle.Fill; Padding = new Padding(10);
        var top = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true };
        top.Controls.AddRange([autoScroll, status]);
        var columns = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 2 };
        columns.ColumnStyles.Add(new(SizeType.Percent, 50)); columns.ColumnStyles.Add(new(SizeType.Percent, 50));
        columns.RowStyles.Add(new(SizeType.AutoSize)); columns.RowStyles.Add(new(SizeType.Percent, 100));
        columns.Controls.Add(new Label { Text = "측정값", AutoSize = true }, 0, 0);
        columns.Controls.Add(new Label { Text = "기타 MCU 데이터 / 상태", AutoSize = true }, 1, 0);
        columns.Controls.Add(measurementLog, 0, 1); columns.Controls.Add(otherLog, 1, 1);
        Controls.Add(columns); Controls.Add(top);
    }
    internal void ClearLog() { measurementLog.Clear(); otherLog.Clear(); status.Text = "STATUS: -"; }
    internal void AddFrame(string frame) => AddOther(frame);
    internal void AddMeasurement(string frame) => Append(measurementLog, frame);
    internal void AddOther(string frame, bool hasStx = true)
    {
        if (frame.StartsWith("STATUS", StringComparison.OrdinalIgnoreCase)) status.Text = frame;
        Append(otherLog, hasStx ? frame : $"[RAW] {frame}");
    }
    private void Append(RichTextBox target, string frame)
    {
        target.AppendText($"[{DateTime.Now:HH:mm:ss.fff}] {frame}{Environment.NewLine}");
        if (autoScroll.Checked) { target.SelectionStart = target.TextLength; target.ScrollToCaret(); }
    }
    private static RichTextBox LogBox() => new() { Dock = DockStyle.Fill, ReadOnly = true,
        WordWrap = false, Font = new Font("Consolas", 9F) };
}
