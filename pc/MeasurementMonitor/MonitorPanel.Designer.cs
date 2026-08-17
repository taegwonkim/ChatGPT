namespace MeasurementMonitor;

partial class MonitorPanel
{
    private System.ComponentModel.IContainer? components;
    private GroupBox monitorGroup = null!;
    private TableLayoutPanel columns = null!;
    private FlowLayoutPanel top = null!;
    private RichTextBox measurementLog = null!;
    private RichTextBox otherLog = null!;
    private CheckBox autoScroll = null!;
    private Label status = null!;

    protected override void Dispose(bool disposing)
    {
        if (disposing) components?.Dispose();
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        components = new System.ComponentModel.Container(); monitorGroup = new GroupBox(); columns = new TableLayoutPanel();
        top = new FlowLayoutPanel(); measurementLog = LogBox(); otherLog = LogBox();
        autoScroll = new CheckBox(); status = new Label(); monitorGroup.SuspendLayout(); columns.SuspendLayout(); top.SuspendLayout(); SuspendLayout();

        autoScroll.AutoSize = true; autoScroll.Checked = true; autoScroll.CheckState = CheckState.Checked;
        autoScroll.Name = "autoScroll"; autoScroll.Text = "Auto scroll";
        status.AutoSize = true; status.Name = "status"; status.Padding = new Padding(5); status.Text = "STATUS: -";
        top.AutoSize = true; top.Dock = DockStyle.Top; top.Controls.Add(autoScroll); top.Controls.Add(status);
        columns.ColumnCount = 2; columns.RowCount = 2; columns.Dock = DockStyle.Fill;
        columns.ColumnStyles.Add(new(SizeType.Percent, 50)); columns.ColumnStyles.Add(new(SizeType.Percent, 50));
        columns.RowStyles.Add(new(SizeType.AutoSize)); columns.RowStyles.Add(new(SizeType.Percent, 100));
        columns.Controls.Add(new Label { Text = "측정값", AutoSize = true }, 0, 0);
        columns.Controls.Add(new Label { Text = "기타 MCU 데이터 / 상태", AutoSize = true }, 1, 0);
        columns.Controls.Add(measurementLog, 0, 1); columns.Controls.Add(otherLog, 1, 1);
        monitorGroup.Controls.Add(columns); monitorGroup.Controls.Add(top); monitorGroup.Dock = DockStyle.Fill;
        monitorGroup.Padding = new Padding(10); monitorGroup.Text = "MCU 수신 데이터"; monitorGroup.Name = "monitorGroup";
        Controls.Add(monitorGroup); AutoScaleMode = AutoScaleMode.Font; Name = "MonitorPanel"; Size = new Size(880, 340);

        top.ResumeLayout(false); top.PerformLayout(); columns.ResumeLayout(false); columns.PerformLayout();
        monitorGroup.ResumeLayout(false); monitorGroup.PerformLayout(); ResumeLayout(false);
    }

    private static RichTextBox LogBox() => new() { Dock = DockStyle.Fill, ReadOnly = true,
        WordWrap = false, Font = new Font("Consolas", 9F) };
}
