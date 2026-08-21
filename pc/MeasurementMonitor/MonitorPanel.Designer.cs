namespace MeasurementMonitor;

partial class MonitorPanel
{
    private System.ComponentModel.IContainer? components;
    private GroupBox monitorGroup = null!;
    private SplitContainer dataSplit = null!;
    private TableLayoutPanel headerLayout = null!;
    private RichTextBox measurementLog = null!;
    private RichTextBox otherLog = null!;
    private CheckBox autoScroll = null!;
    private Label statusCaption = null!;
    private Label status = null!;
    private Label macAddressCaption = null!;
    private Label macAddress = null!;
    private Label measurementTitle = null!;
    private Label otherTitle = null!;

    protected override void Dispose(bool disposing)
    {
        if (disposing) components?.Dispose();
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        components = new System.ComponentModel.Container(); monitorGroup = new GroupBox(); dataSplit = new SplitContainer();
        headerLayout = new TableLayoutPanel(); measurementLog = LogBox(); otherLog = LogBox();
        autoScroll = new CheckBox(); statusCaption = new Label(); status = new Label();
        macAddressCaption = new Label(); macAddress = new Label(); measurementTitle = new Label(); otherTitle = new Label();
        monitorGroup.SuspendLayout(); ((System.ComponentModel.ISupportInitialize)dataSplit).BeginInit();
        dataSplit.Panel1.SuspendLayout(); dataSplit.Panel2.SuspendLayout(); dataSplit.SuspendLayout();
        headerLayout.SuspendLayout(); SuspendLayout();

        autoScroll.AutoSize = true; autoScroll.Checked = true; autoScroll.CheckState = CheckState.Checked;
        autoScroll.Name = "autoScroll"; autoScroll.Text = "Auto scroll";
        statusCaption.AutoSize = true; statusCaption.BackColor = SystemColors.Control;
        statusCaption.ForeColor = SystemColors.ControlText; statusCaption.Margin = new Padding(12, 7, 3, 3);
        statusCaption.Name = "statusCaption"; statusCaption.Text = "STATUS";
        status.AutoSize = false; status.BackColor = Color.White; status.BorderStyle = BorderStyle.FixedSingle;
        status.ForeColor = Color.Black; status.Margin = new Padding(3); status.Name = "status";
        status.Size = new Size(180, 25); status.Text = "-"; status.TextAlign = ContentAlignment.MiddleLeft;
        macAddressCaption.AutoSize = true; macAddressCaption.BackColor = SystemColors.Control;
        macAddressCaption.ForeColor = SystemColors.ControlText; macAddressCaption.Margin = new Padding(3, 7, 3, 3);
        macAddressCaption.Name = "macAddressCaption"; macAddressCaption.Text = "MAC Address";
        macAddress.AutoSize = false; macAddress.BackColor = Color.White; macAddress.BorderStyle = BorderStyle.FixedSingle;
        macAddress.ForeColor = Color.Black; macAddress.Margin = new Padding(3); macAddress.Name = "macAddress";
        macAddress.Size = new Size(180, 25); macAddress.Text = "-"; macAddress.TextAlign = ContentAlignment.MiddleLeft;
        headerLayout.AutoSize = true; headerLayout.BackColor = SystemColors.Control; headerLayout.ColumnCount = 7;
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 80F));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        headerLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
        headerLayout.Controls.Add(autoScroll, 0, 0); headerLayout.Controls.Add(statusCaption, 1, 0);
        headerLayout.Controls.Add(status, 2, 0); headerLayout.Controls.Add(macAddressCaption, 4, 0);
        headerLayout.Controls.Add(macAddress, 5, 0); headerLayout.Dock = DockStyle.Top;
        headerLayout.Name = "headerLayout"; headerLayout.RowCount = 1;
        headerLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        measurementTitle.AutoSize = true; measurementTitle.Dock = DockStyle.Top;
        measurementTitle.Name = "measurementTitle"; measurementTitle.Text = "측정값";
        otherTitle.AutoSize = true; otherTitle.Dock = DockStyle.Top;
        otherTitle.Name = "otherTitle"; otherTitle.Text = "기타 MCU 데이터 / 상태";
        dataSplit.BackColor = SystemColors.ControlDark; dataSplit.BorderStyle = BorderStyle.FixedSingle;
        dataSplit.Cursor = Cursors.VSplit; dataSplit.Dock = DockStyle.Fill;
        dataSplit.IsSplitterFixed = false; dataSplit.Name = "dataSplit";
        dataSplit.Orientation = Orientation.Vertical;
        dataSplit.Panel1.Controls.Add(measurementLog); dataSplit.Panel1.Controls.Add(measurementTitle);
        dataSplit.Panel1.BackColor = SystemColors.Control; dataSplit.Panel1MinSize = 150;
        dataSplit.Panel2.Controls.Add(otherLog); dataSplit.Panel2.Controls.Add(otherTitle);
        dataSplit.Panel2.BackColor = SystemColors.Control; dataSplit.Panel2MinSize = 150;
        dataSplit.Size = new Size(860, 280);
        dataSplit.SplitterDistance = 428; dataSplit.SplitterWidth = 6; dataSplit.TabIndex = 1;
        monitorGroup.Controls.Add(dataSplit); monitorGroup.Controls.Add(headerLayout); monitorGroup.Dock = DockStyle.Fill;
        monitorGroup.Padding = new Padding(10); monitorGroup.Text = "MCU 수신 데이터"; monitorGroup.Name = "monitorGroup";
        Controls.Add(monitorGroup); AutoScaleMode = AutoScaleMode.Font; Name = "MonitorPanel"; Size = new Size(880, 340);

        headerLayout.ResumeLayout(false); headerLayout.PerformLayout();
        dataSplit.Panel1.ResumeLayout(false); dataSplit.Panel1.PerformLayout();
        dataSplit.Panel2.ResumeLayout(false); dataSplit.Panel2.PerformLayout();
        ((System.ComponentModel.ISupportInitialize)dataSplit).EndInit(); dataSplit.ResumeLayout(false);
        monitorGroup.ResumeLayout(false); monitorGroup.PerformLayout(); ResumeLayout(false);
    }

    private static RichTextBox LogBox() => new RichTextBox { Dock = DockStyle.Fill, ReadOnly = true,
        WordWrap = false, Font = new Font("Consolas", 9F) };
}
