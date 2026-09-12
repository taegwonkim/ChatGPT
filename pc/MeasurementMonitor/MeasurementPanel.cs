namespace MeasurementMonitor;

public partial class MeasurementPanel : UserControl
{
    internal event EventHandler? ReadRequested;
    internal event EventHandler<MeasurementSettings>? WriteRequested;
    internal event EventHandler<ResetTimeUnit>? ResetReadRequested;
    internal event EventHandler<ResetPeriodSetting>? ResetWriteRequested;

    public MeasurementPanel()
    {
        InitializeComponent();
        readButton.Click += (_, _) => ReadRequested?.Invoke(this, EventArgs.Empty);
        writeButton.Click += (_, _) => WriteRequested?.Invoke(this,
            new(reference.Value, offset.Value, resistance.Value, interval.Value,
                rs485Only.SelectedIndex == 1));
        resetReadButton.Click += (_, _) => ResetReadRequested?.Invoke(this, SelectedResetUnit);
        resetWriteButton.Click += (_, _) => WriteResetInterval();
        resetUnit.SelectedIndexChanged += (_, _) => UpdateResetMaximum();
    }

    internal void Apply(MeasurementSettings value)
    {
        SetValueWithinRange(reference, value.ReferenceMv);
        SetValueWithinRange(offset, value.OffsetMv);
        SetValueWithinRange(resistance, value.ResistanceMilliOhm);
        SetValueWithinRange(interval, value.IntervalSeconds);
        rs485Only.SelectedIndex = value.Rs485Only ? 1 : 0;
    }
    internal MeasurementSettings CurrentSettings => new(reference.Value, offset.Value,
        resistance.Value, interval.Value, rs485Only.SelectedIndex == 1);
    internal uint ResetIntervalSeconds => ToResetSeconds(resetInterval.Value);
    internal void ApplyResetPeriod(ResetPeriodSetting setting)
    {
        resetUnit.SelectedIndex = setting.Unit == ResetTimeUnit.Hours ? 1 : 0;
        UpdateResetMaximum();
        SetValueWithinRange(resetInterval, setting.Value);
    }
    internal void ApplyResetInterval(uint seconds)
    {
        bool useHours = seconds >= 3600U && seconds % 3600U == 0U;
        resetUnit.SelectedIndex = useHours ? 1 : 0;
        UpdateResetMaximum();
        uint divisor = useHours ? 3600U : 60U;
        SetValueWithinRange(resetInterval, seconds / divisor);
    }

    private void WriteResetInterval()
    {
        ResetWriteRequested?.Invoke(this,
            new(decimal.ToUInt32(resetInterval.Value), SelectedResetUnit));
    }

    private ResetTimeUnit SelectedResetUnit =>
        resetUnit.SelectedIndex == 1 ? ResetTimeUnit.Hours : ResetTimeUnit.Minutes;
    private decimal ResetUnitSeconds => resetUnit.SelectedIndex == 1 ? 3600M : 60M;
    private uint ToResetSeconds(decimal value) => decimal.ToUInt32(value * ResetUnitSeconds);
    private void UpdateResetMaximum()
    {
        decimal maximum = resetUnit.SelectedIndex == 1 ? 8760M : 525600M;
        resetInterval.Maximum = maximum;
        if (resetInterval.Value > maximum) resetInterval.Value = maximum;
    }

    private static void SetValueWithinRange(NumericUpDown control, decimal requestedValue)
    {
        if (control.IsDisposed || control.Disposing) return;

        // Minimum/Maximum을 먼저 snapshot한 뒤 범위를 제한합니다. 저장 파일이나 MCU
        // 응답값이 UI 범위를 벗어나도 NumericUpDown.Value에는 유효한 값만 전달됩니다.
        decimal minimum = control.Minimum;
        decimal maximum = control.Maximum;
        decimal boundedValue = Math.Min(maximum, Math.Max(minimum, requestedValue));
        if (control.Value != boundedValue)
            control.Value = boundedValue;
    }

}
