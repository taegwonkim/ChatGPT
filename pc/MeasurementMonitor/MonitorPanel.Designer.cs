namespace MeasurementMonitor;

partial class MonitorPanel
{
    private System.ComponentModel.IContainer? components;
    private GroupBox monitorGroup = null!;
    private TableLayoutPanel columns = null!;
    private TableLayoutPanel headerLayout = null!;
    private RichTextBox measurementLog = null!;
    private RichTextBox otherLog = null!;
    private CheckBox autoScroll = null!;
    private Label status = null!;
    private Label macAddress = null!;

    protected override void Dispose(bool disposing)
    {
        if (disposing) components?.Dispose();
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        components = new System.ComponentModel.Container(); monitorGroup = new GroupBox(); columns = new TableLayoutPanel();
        headerLayout = new TableLayoutPanel(); measurementLog = LogBox(); otherLog = LogBox();
        autoScroll = new CheckBox(); status = new Label(); macAddress = new Label(); monitorGroup.SuspendLayout(); columns.SuspendLayout(); headerLayout.SuspendLayout(); SuspendLayout();

        autoScroll.AutoSize = true; autoScroll.Checked = true; autoScroll.CheckState = CheckState.Checked;
        autoScroll.Name = "autoScroll"; autoScroll.Text = "Auto scroll";
        status.AutoSize = true; status.BackColor = Color.White; status.ForeColor = Color.Black;
        status.Margin = new Padding(12, 3, 3, 3); status.Name = "status"; status.Padding = new Padding(5); status.Text = "STATUS: -";
        macAddress.AutoSize = true; macAddress.BackColor = Color.White; macAddress.ForeColor = Color.Black;
        macAddress.Margin = new Padding(3); macAddress.Name = "macAddress";
        macAddress.Padding = new Padding(5); macAddress.Text = "MAC Address: -";
        headerLayout.AutoSize = true; headerLayout.BackColor = Color.White; headerLayout.ColumnCount = 5;
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 80F));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
        headerLayout.Controls.Add(autoScroll, 0, 0); headerLayout.Controls.Add(status, 1, 0);
        headerLayout.Controls.Add(macAddress, 3, 0); headerLayout.Dock = DockStyle.Top;
        headerLayout.Name = "headerLayout"; headerLayout.RowCount = 1;
        headerLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        columns.ColumnCount = 2; columns.RowCount = 2; columns.Dock = DockStyle.Fill;
        columns.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F)); columns.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
        columns.RowStyles.Add(new RowStyle(SizeType.AutoSize)); columns.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
        columns.Controls.Add(new Label { Text = "측정값", AutoSize = true }, 0, 0);
        columns.Controls.Add(new Label { Text = "기타 MCU 데이터 / 상태", AutoSize = true }, 1, 0);
        columns.Controls.Add(measurementLog, 0, 1); columns.Controls.Add(otherLog, 1, 1);
        monitorGroup.Controls.Add(columns); monitorGroup.Controls.Add(headerLayout); monitorGroup.Dock = DockStyle.Fill;
        monitorGroup.Padding = new Padding(10); monitorGroup.Text = "MCU 수신 데이터"; monitorGroup.Name = "monitorGroup";
        Controls.Add(monitorGroup); AutoScaleMode = AutoScaleMode.Font; Name = "MonitorPanel"; Size = new Size(880, 340);

        headerLayout.ResumeLayout(false); headerLayout.PerformLayout(); columns.ResumeLayout(false); columns.PerformLayout();
        monitorGroup.ResumeLayout(false); monitorGroup.PerformLayout(); ResumeLayout(false);
    }

    private static RichTextBox LogBox() => new RichTextBox { Dock = DockStyle.Fill, ReadOnly = true,
        WordWrap = false, Font = new Font("Consolas", 9F) };
}
