namespace MeasurementMonitor;

partial class SerialPanel
{
    private System.ComponentModel.IContainer? components;
    private GroupBox serialGroup = null!;
    private FlowLayoutPanel row = null!;
    private ComboBox ports = null!;
    private ComboBox baud = null!;
    private NumericUpDown timeout = null!;
    private Button openClose = null!;
    private Button refresh = null!;
    private Button clear = null!;

    protected override void Dispose(bool disposing)
    {
        if (disposing) components?.Dispose();
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        components = new System.ComponentModel.Container();
        serialGroup = new GroupBox(); row = new FlowLayoutPanel();
        ports = new ComboBox(); baud = new ComboBox(); timeout = new NumericUpDown();
        openClose = new Button(); refresh = new Button(); clear = new Button();
        ((System.ComponentModel.ISupportInitialize)timeout).BeginInit();
        serialGroup.SuspendLayout(); row.SuspendLayout(); SuspendLayout();

        ports.DropDownStyle = ComboBoxStyle.DropDownList; ports.Name = "ports"; ports.Width = 100;
        baud.DropDownStyle = ComboBoxStyle.DropDownList; baud.Name = "baud"; baud.Width = 90;
        baud.Items.AddRange(new object[] { "9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600" });
        baud.SelectedItem = "115200";
        timeout.Minimum = 10; timeout.Maximum = 60000; timeout.Value = 1000; timeout.Name = "timeout"; timeout.Width = 90;
        refresh.AutoSize = true; refresh.Name = "refresh"; refresh.Text = "새로고침";
        openClose.AutoSize = true; openClose.Name = "openClose"; openClose.Text = "Open";
        clear.AutoSize = true; clear.Name = "clear"; clear.Text = "Clear";

        row.AutoSize = true; row.Dock = DockStyle.Fill; row.Name = "row"; row.WrapContents = true;
        row.Controls.Add(MakeLabel("COM Port")); row.Controls.Add(ports); row.Controls.Add(refresh);
        row.Controls.Add(MakeLabel("Baudrate")); row.Controls.Add(baud);
        row.Controls.Add(MakeLabel("Timeout(ms)")); row.Controls.Add(timeout);
        row.Controls.Add(openClose); row.Controls.Add(clear);
        serialGroup.Controls.Add(row); serialGroup.Dock = DockStyle.Fill; serialGroup.Name = "serialGroup";
        serialGroup.Padding = new Padding(10); serialGroup.Text = "Serial Port";
        Controls.Add(serialGroup); AutoScaleMode = AutoScaleMode.Font; Name = "SerialPanel"; Size = new Size(880, 75);

        ((System.ComponentModel.ISupportInitialize)timeout).EndInit();
        row.ResumeLayout(false); row.PerformLayout(); serialGroup.ResumeLayout(false); serialGroup.PerformLayout(); ResumeLayout(false);
    }

    private static Label MakeLabel(string text) => new Label { AutoSize = true, Text = text, Margin = new Padding(10, 7, 3, 3) };
}
