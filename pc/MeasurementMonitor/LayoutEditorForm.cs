namespace MeasurementMonitor;

internal sealed class LayoutEditorForm : Form
{
    private readonly RuntimeLayoutManager manager;
    private readonly TreeView controlsTree = new() { Dock = DockStyle.Left, Width = 300 };
    private readonly PropertyGrid properties = new() { Dock = DockStyle.Fill };

    internal LayoutEditorForm(RuntimeLayoutManager layoutManager)
    {
        manager = layoutManager;
        Text = "화면 배치 편집기";
        StartPosition = FormStartPosition.CenterParent;
        MinimumSize = new Size(760, 520);
        var buttons = new FlowLayoutPanel { Dock = DockStyle.Bottom, AutoSize = true };
        buttons.Controls.Add(MakeButton("자유 배치", FreeSelected));
        buttons.Controls.Add(MakeButton("저장", Save));
        buttons.Controls.Add(MakeButton("초기화", Reset));
        buttons.Controls.Add(MakeButton("닫기", Close));
        Controls.Add(properties);
        Controls.Add(controlsTree);
        Controls.Add(buttons);
        controlsTree.AfterSelect += (_, args) =>
        {
            properties.SelectedObject = args.Node.Tag;
            if (args.Node.Tag is Control control) manager.ShowSelection(control);
        };
        FormClosed += (_, _) => manager.ClearSelection();
        PopulateTree();
    }

    private void PopulateTree()
    {
        foreach (KeyValuePair<string, Control> item in manager.Controls.OrderBy(item => item.Key))
        {
            if (item.Value is not (Label or TextBox or RichTextBox or ComboBox or Button or
                CheckBox or NumericUpDown or SplitContainer or TableLayoutPanel or UserControl)) continue;
            string text = string.IsNullOrWhiteSpace(item.Value.Text)
                ? $"{item.Value.GetType().Name} ({item.Value.Name})"
                : $"{item.Value.Text} ({item.Value.Name})";
            controlsTree.Nodes.Add(new TreeNode(text) { Tag = item.Value });
        }
    }

    private void FreeSelected()
    {
        if (properties.SelectedObject is not Control control) return;
        manager.Free(control);
        properties.Refresh();
    }

    private void Save() => MessageBox.Show(manager.Save() ?
        "현재 배치를 저장했습니다." : "배치를 저장하지 못했습니다.");

    private void Reset()
    {
        manager.Reset();
        MessageBox.Show("저장된 사용자 배치를 삭제했습니다. 프로그램을 다시 시작하십시오.");
    }

    private static Button MakeButton(string text, Action action)
    {
        var button = new Button { Text = text, AutoSize = true };
        button.Click += (_, _) => action();
        return button;
    }
}
