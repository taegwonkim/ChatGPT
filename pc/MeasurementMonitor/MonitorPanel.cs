namespace MeasurementMonitor;

public class MonitorPanel : GroupBox
{
    private readonly RichTextBox log = new() { Dock = DockStyle.Fill, ReadOnly = true, WordWrap = false };
    private readonly CheckBox autoScroll = new() { Text = "Auto scroll", Checked = true, AutoSize = true };
    private readonly Label status = new() { Text = "STATUS: -", AutoSize = true, Padding = new Padding(5) };

    public MonitorPanel()
    {
        Text = "측정값 / 상태"; Dock = DockStyle.Fill; Padding = new Padding(10);
        var top = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true };
        top.Controls.AddRange([autoScroll, status]);
        Controls.Add(log); Controls.Add(top);
    }
    internal void ClearLog() { log.Clear(); status.Text = "STATUS: -"; }
    internal void AddFrame(string frame)
    {
        if (frame.StartsWith("STATUS", StringComparison.OrdinalIgnoreCase)) status.Text = frame;
        log.AppendText($"[{DateTime.Now:HH:mm:ss.fff}] {frame}{Environment.NewLine}");
        if (autoScroll.Checked) { log.SelectionStart = log.TextLength; log.ScrollToCaret(); }
    }
}
