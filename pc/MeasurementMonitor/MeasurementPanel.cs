namespace MeasurementMonitor;

public partial class MeasurementPanel : UserControl
{
    internal event EventHandler? ReadRequested;
    internal event EventHandler<MeasurementSettings>? WriteRequested;

    public MeasurementPanel()
    {
        InitializeComponent();
        readButton.Click += (_, _) => ReadRequested?.Invoke(this, EventArgs.Empty);
        writeButton.Click += (_, _) => WriteRequested?.Invoke(this,
            new(reference.Value, offset.Value, resistance.Value, interval.Value,
                rs485Only.SelectedIndex == 1));
    }

    internal void Apply(MeasurementSettings value)
    {
        reference.Value = Clamp(reference, value.ReferenceMv);
        offset.Value = Clamp(offset, value.OffsetMv);
        resistance.Value = Clamp(resistance, value.ResistanceMilliOhm);
        interval.Value = Clamp(interval, value.IntervalSeconds);
        rs485Only.SelectedIndex = value.Rs485Only ? 1 : 0;
    }
    internal MeasurementSettings CurrentSettings => new(reference.Value, offset.Value,
        resistance.Value, interval.Value, rs485Only.SelectedIndex == 1);

    private static decimal Clamp(NumericUpDown control, decimal value) =>
        Math.Min(control.Maximum, Math.Max(control.Minimum, value));

}
