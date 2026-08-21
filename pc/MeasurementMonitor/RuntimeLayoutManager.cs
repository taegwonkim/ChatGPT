using System.Text.Json;

namespace MeasurementMonitor;

internal sealed record ControlLayout(string Key, string HostKey, int X, int Y, int Width,
    int Height, string FontName, float FontSize, int FontStyle, int BackColor,
    int ForeColor, int Anchor);

internal sealed class RuntimeLayoutManager
{
    private readonly Control root;
    private readonly Dictionary<string, Control> controls = [];
    private readonly Dictionary<Control, string> keys = [];
    private readonly HashSet<Control> freed = [];
    private static string FilePath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "STM32MeasurementMonitor", "control-layout.json");

    internal IEnumerable<KeyValuePair<string, Control>> Controls => controls;

    internal RuntimeLayoutManager(Control rootControl)
    {
        root = rootControl;
        controls["root"] = root;
        keys[root] = "root";
        Index(root, "root");
    }

    private void Index(Control parent, string parentKey)
    {
        for (int index = 0; index < parent.Controls.Count; index++)
        {
            Control child = parent.Controls[index];
            string id = string.IsNullOrWhiteSpace(child.Name) ? $"{child.GetType().Name}{index}" : child.Name;
            string key = $"{parentKey}/{id}";
            controls[key] = child;
            keys[child] = key;
            Index(child, key);
        }
    }

    internal void ApplySaved()
    {
        try
        {
            if (!File.Exists(FilePath)) return;
            ControlLayout[] layouts = JsonSerializer.Deserialize<ControlLayout[]>(File.ReadAllText(FilePath)) ?? [];
            foreach (ControlLayout layout in layouts)
            {
                if (!controls.TryGetValue(layout.Key, out Control? control) ||
                    !controls.TryGetValue(layout.HostKey, out Control? host)) continue;
                Reparent(control, host, new Rectangle(layout.X, layout.Y,
                    Math.Max(10, layout.Width), Math.Max(10, layout.Height)));
                if (layout.FontSize > 0)
                    control.Font = new Font(layout.FontName, layout.FontSize,
                        (FontStyle)layout.FontStyle, GraphicsUnit.Point);
                if (layout.BackColor != 0) control.BackColor = Color.FromArgb(layout.BackColor);
                if (layout.ForeColor != 0) control.ForeColor = Color.FromArgb(layout.ForeColor);
                control.Anchor = (AnchorStyles)layout.Anchor;
            }
        }
        catch (JsonException) { }
        catch (IOException) { }
        catch (ArgumentException) { }
    }

    internal void Free(Control control)
    {
        Control host = control is UserControl ? root : FindHost(control);
        Rectangle screen = control.RectangleToScreen(new Rectangle(Point.Empty, control.Size));
        Point location = host.PointToClient(screen.Location);
        Reparent(control, host, new Rectangle(location, screen.Size));
    }

    private Control FindHost(Control control)
    {
        for (Control? candidate = control.Parent; candidate is not null; candidate = candidate.Parent)
            if (candidate is GroupBox && keys.ContainsKey(candidate)) return candidate;
        return root;
    }

    private void Reparent(Control control, Control host, Rectangle bounds)
    {
        control.Parent?.Controls.Remove(control);
        host.Controls.Add(control);
        control.Dock = DockStyle.None;
        control.Anchor = AnchorStyles.Top | AnchorStyles.Left;
        control.Bounds = bounds;
        control.BringToFront();
        freed.Add(control);
    }

    internal bool Save()
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
            ControlLayout[] layouts = freed.Where(control => control.Parent is not null && keys.ContainsKey(control.Parent))
                .Select(control => new ControlLayout(keys[control], keys[control.Parent!], control.Left,
                    control.Top, control.Width, control.Height, control.Font.Name, control.Font.Size,
                    (int)control.Font.Style, control.BackColor.ToArgb(), control.ForeColor.ToArgb(),
                    (int)control.Anchor)).ToArray();
            File.WriteAllText(FilePath, JsonSerializer.Serialize(layouts,
                new JsonSerializerOptions { WriteIndented = true }));
            return true;
        }
        catch (IOException) { return false; }
        catch (UnauthorizedAccessException) { return false; }
    }

    internal void Reset()
    {
        try { if (File.Exists(FilePath)) File.Delete(FilePath); }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
    }
}
