namespace MeasurementMonitor;

partial class MeasurementPanel
{
    private System.ComponentModel.IContainer? components;
    private GroupBox measurementGroup = null!;
    private TableLayoutPanel grid = null!;
    private NumericUpDown reference = null!;
    private NumericUpDown offset = null!;
    private NumericUpDown resistance = null!;
    private NumericUpDown interval = null!;
    private Button readButton = null!;
    private Button writeButton = null!;

    protected override void Dispose(bool disposing)
    {
        if (disposing) components?.Dispose();
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        components = new System.ComponentModel.Container(); measurementGroup = new GroupBox(); grid = new TableLayoutPanel();
        reference = Number(0, 1000000, 1, 0); offset = Number(-1000000, 1000000, 1, 0);
        resistance = Number(0, 100000000, 3, 0); interval = Number(0.001M, 86400, 3, 1);
        readButton = new Button(); writeButton = new Button(); measurementGroup.SuspendLayout(); grid.SuspendLayout(); SuspendLayout();

        grid.ColumnCount = 4; grid.RowCount = 3; grid.Dock = DockStyle.Fill;
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120F)); grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 140F)); grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
        grid.Controls.Add(MakeLabel("Reference (mV)"), 0, 0); grid.Controls.Add(reference, 1, 0);
        grid.Controls.Add(MakeLabel("Offset (mV)"), 2, 0); grid.Controls.Add(offset, 3, 0);
        grid.Controls.Add(MakeLabel("Resistance (mΩ)"), 0, 1); grid.Controls.Add(resistance, 1, 1);
        grid.Controls.Add(MakeLabel("Interval Time (sec)"), 2, 1); grid.Controls.Add(interval, 3, 1);
        readButton.AutoSize = true; readButton.Anchor = AnchorStyles.None; readButton.Text = "Read"; readButton.Name = "readButton";
        writeButton.AutoSize = true; writeButton.Anchor = AnchorStyles.None; writeButton.Text = "Write"; writeButton.Name = "writeButton";
        grid.Controls.Add(readButton, 1, 2); grid.Controls.Add(writeButton, 2, 2);
        measurementGroup.Controls.Add(grid); measurementGroup.Dock = DockStyle.Fill; measurementGroup.Padding = new Padding(10);
        measurementGroup.Text = "Measurement 설정"; measurementGroup.Name = "measurementGroup";
        Controls.Add(measurementGroup); AutoScaleMode = AutoScaleMode.Font; Name = "MeasurementPanel"; Size = new Size(520, 230);

        grid.ResumeLayout(false); grid.PerformLayout(); measurementGroup.ResumeLayout(false); ResumeLayout(false);
    }

    private static NumericUpDown Number(decimal min, decimal max, int decimals, decimal value) =>
        new NumericUpDown { Minimum = min, Maximum = max, DecimalPlaces = decimals, Value = value, ThousandsSeparator = true, Dock = DockStyle.Fill };
    private static Label MakeLabel(string text) => new Label { Anchor = AnchorStyles.Left, AutoSize = true, Text = text };
}
