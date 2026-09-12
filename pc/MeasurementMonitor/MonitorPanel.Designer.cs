namespace MeasurementMonitor;

partial class MonitorPanel
{
    private System.ComponentModel.IContainer? components;
    private GroupBox monitorGroup = null!;
    private TableLayoutPanel monitorLayout = null!;
    private SplitContainer dataSplit = null!;
    private TableLayoutPanel headerLayout = null!;
    private TableLayoutPanel measurementLayout = null!;
    private TableLayoutPanel otherLayout = null!;
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
        components = new System.ComponentModel.Container(); monitorGroup = new GroupBox();
        monitorLayout = new TableLayoutPanel(); dataSplit = new SplitContainer();
        headerLayout = new TableLayoutPanel(); measurementLayout = new TableLayoutPanel();
        otherLayout = new TableLayoutPanel(); measurementLog = LogBox(); otherLog = LogBox();
        autoScroll = new CheckBox(); statusCaption = new Label(); status = new Label();
        macAddressCaption = new Label(); macAddress = new Label(); measurementTitle = new Label(); otherTitle = new Label();
        monitorGroup.SuspendLayout(); monitorLayout.SuspendLayout();
        ((System.ComponentModel.ISupportInitialize)dataSplit).BeginInit();
        dataSplit.Panel1.SuspendLayout(); dataSplit.Panel2.SuspendLayout(); dataSplit.SuspendLayout();
        headerLayout.SuspendLayout(); measurementLayout.SuspendLayout(); otherLayout.SuspendLayout(); SuspendLayout();

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
        headerLayout.Controls.Add(macAddress, 5, 0); headerLayout.Dock = DockStyle.Fill;
        headerLayout.Name = "headerLayout"; headerLayout.RowCount = 1;
        headerLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        measurementTitle.AutoSize = true; measurementTitle.Dock = DockStyle.Fill;
        measurementTitle.Margin = new Padding(0, 0, 0, 3);
        measurementTitle.Name = "measurementTitle"; measurementTitle.Text = "측정값";
        otherTitle.AutoSize = true; otherTitle.Dock = DockStyle.Fill;
        otherTitle.Margin = new Padding(0, 0, 0, 3);
        otherTitle.Name = "otherTitle"; otherTitle.Text = "기타 MCU 데이터 / 상태";
        measurementLayout.ColumnCount = 1; measurementLayout.Dock = DockStyle.Fill;
        measurementLayout.Margin = Padding.Empty; measurementLayout.Name = "measurementLayout";
        measurementLayout.RowCount = 2;
        measurementLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        measurementLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
        measurementLayout.Controls.Add(measurementTitle, 0, 0);
        measurementLayout.Controls.Add(measurementLog, 0, 1);
        otherLayout.ColumnCount = 1; otherLayout.Dock = DockStyle.Fill;
        otherLayout.Margin = Padding.Empty; otherLayout.Name = "otherLayout";
        otherLayout.RowCount = 2;
        otherLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        otherLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
        otherLayout.Controls.Add(otherTitle, 0, 0); otherLayout.Controls.Add(otherLog, 0, 1);
        dataSplit.BackColor = SystemColors.ControlDark; dataSplit.BorderStyle = BorderStyle.FixedSingle;
        dataSplit.Cursor = Cursors.VSplit; dataSplit.Dock = DockStyle.Fill;
        dataSplit.IsSplitterFixed = false; dataSplit.Name = "dataSplit";
        dataSplit.Orientation = Orientation.Vertical;
        dataSplit.Panel1.Controls.Add(measurementLayout);
        dataSplit.Panel1.BackColor = SystemColors.Control; dataSplit.Panel1MinSize = 150;
        dataSplit.Panel2.Controls.Add(otherLayout);
        dataSplit.Panel2.BackColor = SystemColors.Control; dataSplit.Panel2MinSize = 150;
        dataSplit.Size = new Size(860, 280);
        dataSplit.SplitterDistance = 428; dataSplit.SplitterWidth = 6; dataSplit.TabIndex = 1;
        monitorLayout.ColumnCount = 1; monitorLayout.Dock = DockStyle.Fill;
        monitorLayout.Margin = Padding.Empty; monitorLayout.Name = "monitorLayout";
        monitorLayout.RowCount = 2;
        monitorLayout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        monitorLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
        monitorLayout.Controls.Add(headerLayout, 0, 0); monitorLayout.Controls.Add(dataSplit, 0, 1);
        monitorGroup.Controls.Add(monitorLayout); monitorGroup.Dock = DockStyle.Fill;
        monitorGroup.Padding = new Padding(10); monitorGroup.Text = "MCU 수신 데이터"; monitorGroup.Name = "monitorGroup";
        Controls.Add(monitorGroup); AutoScaleMode = AutoScaleMode.Font; Name = "MonitorPanel"; Size = new Size(880, 340);

        headerLayout.ResumeLayout(false); headerLayout.PerformLayout();
        measurementLayout.ResumeLayout(false); measurementLayout.PerformLayout();
        otherLayout.ResumeLayout(false); otherLayout.PerformLayout();
        dataSplit.Panel1.ResumeLayout(false); dataSplit.Panel2.ResumeLayout(false);
        ((System.ComponentModel.ISupportInitialize)dataSplit).EndInit(); dataSplit.ResumeLayout(false);
        monitorLayout.ResumeLayout(false); monitorLayout.PerformLayout();
        monitorGroup.ResumeLayout(false); monitorGroup.PerformLayout(); ResumeLayout(false);
    }

    private static RichTextBox LogBox() => new RichTextBox { BorderStyle = BorderStyle.FixedSingle,
        DetectUrls = false, Dock = DockStyle.Fill, Margin = Padding.Empty, ReadOnly = true,
        WordWrap = false, Font = new Font("Consolas", 9F) };
}
