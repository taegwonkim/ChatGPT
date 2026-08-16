namespace MeasurementMonitor;

internal sealed class MeasurementPanel : GroupBox
{
    private readonly NumericUpDown reference = Number(0, 1000000, 1);
    private readonly NumericUpDown offset = Number(-1000000, 1000000, 1);
    private readonly NumericUpDown resistance = Number(0, 100000000, 3);
    private readonly NumericUpDown interval = Number(0.001m, 86400, 3, 1);
    internal event EventHandler? ReadRequested;
    internal event EventHandler<MeasurementSettings>? WriteRequested;

    internal MeasurementPanel()
    {
        Text = "Measurement 설정"; Dock = DockStyle.Fill; Padding = new Padding(10);
        var grid = Ui.Grid();
        Ui.AddRow(grid, "Reference (mV)", reference, "Offset (mV)", offset);
        Ui.AddRow(grid, "Resistance (mΩ)", resistance, "Interval Time (sec)", interval);
        var buttons = Ui.Buttons(("Read", () => ReadRequested?.Invoke(this, EventArgs.Empty)),
            ("Write", () => WriteRequested?.Invoke(this,
                new(reference.Value, offset.Value, resistance.Value, interval.Value))));
        grid.Controls.Add(buttons, 0, 2); grid.SetColumnSpan(buttons, 4);
        Controls.Add(grid);
    }

    internal void Apply(MeasurementSettings value)
    {
        reference.Value = Clamp(reference, value.ReferenceMv);
        offset.Value = Clamp(offset, value.OffsetMv);
        resistance.Value = Clamp(resistance, value.ResistanceMilliOhm);
        interval.Value = Clamp(interval, value.IntervalSeconds);
    }

    private static decimal Clamp(NumericUpDown control, decimal value) =>
        Math.Min(control.Maximum, Math.Max(control.Minimum, value));

    private static NumericUpDown Number(decimal min, decimal max, int decimals, decimal value = 0) =>
        new() { Minimum = min, Maximum = max, DecimalPlaces = decimals, Value = value, ThousandsSeparator = true };
}

internal static class Ui
{
    internal static TableLayoutPanel Grid() => new() { Dock = DockStyle.Fill, AutoSize = true,
        ColumnCount = 4, RowCount = 0, AutoScroll = true };
    internal static void AddRow(TableLayoutPanel grid, string a, Control ac, string b, Control bc)
    {
        int row = grid.RowCount++;
        grid.RowStyles.Add(new(SizeType.AutoSize));
        grid.Controls.Add(new Label { Text = a, AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        ac.Dock = DockStyle.Fill; grid.Controls.Add(ac, 1, row);
        grid.Controls.Add(new Label { Text = b, AutoSize = true, Anchor = AnchorStyles.Left }, 2, row);
        bc.Dock = DockStyle.Fill; grid.Controls.Add(bc, 3, row);
    }
    internal static FlowLayoutPanel Buttons(params (string Text, Action Click)[] definitions)
    {
        var panel = new FlowLayoutPanel { AutoSize = true, Dock = DockStyle.Fill };
        foreach (var item in definitions) { var button = new Button { Text = item.Text, AutoSize = true };
            button.Click += (_, _) => item.Click(); panel.Controls.Add(button); }
        return panel;
    }
}
