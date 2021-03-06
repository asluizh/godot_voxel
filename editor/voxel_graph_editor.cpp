#include "voxel_graph_editor.h"
#include "../generators/graph/voxel_generator_graph.h"
#include "editor/editor_scale.h"
#include <core/undo_redo.h>
#include <scene/gui/graph_edit.h>
#include <scene/gui/label.h>

class VoxelGraphEditorNode : public GraphNode {
	GDCLASS(VoxelGraphEditorNode, GraphNode)
public:
	uint32_t node_id = 0;
	std::vector<Control *> input_controls;
	std::vector<Control *> param_controls;
};

VoxelGraphEditor::VoxelGraphEditor() {
	_graph_edit = memnew(GraphEdit);
	_graph_edit->set_anchors_preset(Control::PRESET_WIDE);
	_graph_edit->set_right_disconnects(true);
	_graph_edit->connect("gui_input", this, "_on_graph_edit_gui_input");
	_graph_edit->connect("connection_request", this, "_on_graph_edit_connection_request");
	_graph_edit->connect("delete_nodes_request", this, "_on_graph_edit_delete_nodes_request");
	_graph_edit->connect("disconnection_request", this, "_on_graph_edit_disconnection_request");
	add_child(_graph_edit);

	_context_menu = memnew(PopupMenu);
	for (int i = 0; i < VoxelGraphNodeDB::get_singleton()->get_type_count(); ++i) {
		const VoxelGraphNodeDB::NodeType &node_type = VoxelGraphNodeDB::get_singleton()->get_type(i);
		_context_menu->add_item(node_type.name);
		_context_menu->set_item_metadata(i, i);
	}
	_context_menu->connect("index_pressed", this, "_on_context_menu_index_pressed");
	_context_menu->hide();
	add_child(_context_menu);
}

void VoxelGraphEditor::set_graph(Ref<VoxelGeneratorGraph> graph) {
	if (_graph == graph) {
		return;
	}

	//	if (_graph.is_valid()) {
	//	}

	_graph = graph;

	//	if (_graph.is_valid()) {
	//	}

	build_gui_from_graph();
}

void VoxelGraphEditor::set_undo_redo(UndoRedo *undo_redo) {
	_undo_redo = undo_redo;
}

void VoxelGraphEditor::clear() {
	_graph_edit->clear_connections();
	for (int i = 0; i < _graph_edit->get_child_count(); ++i) {
		Node *node = _graph_edit->get_child(i);
		GraphNode *node_view = Object::cast_to<GraphNode>(node);
		if (node_view != nullptr) {
			memdelete(node_view);
			--i;
		}
	}
}

inline String node_to_gui_name(uint32_t node_id) {
	return String("{0}").format(varray(node_id));
}

void VoxelGraphEditor::build_gui_from_graph() {
	// Rebuild the entire graph GUI

	clear();

	if (_graph.is_null()) {
		return;
	}

	const VoxelGeneratorGraph &graph = **_graph;

	// Nodes

	PoolIntArray node_ids = graph.get_node_ids();
	{
		PoolIntArray::Read node_ids_read = node_ids.read();
		for (int i = 0; i < node_ids.size(); ++i) {
			const uint32_t node_id = node_ids_read[i];
			create_node_gui(node_id);
		}
	}

	// Connections

	std::vector<ProgramGraph::Connection> connections;
	graph.get_connections(connections);

	for (size_t i = 0; i < connections.size(); ++i) {
		const ProgramGraph::Connection &con = connections[i];
		const String from_node_name = node_to_gui_name(con.src.node_id);
		const String to_node_name = node_to_gui_name(con.dst.node_id);
		VoxelGraphEditorNode *to_node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_node(to_node_name));
		ERR_FAIL_COND(to_node_view == nullptr);
		const Error err = _graph_edit->connect_node(from_node_name, con.src.port_index, to_node_view->get_name(), con.dst.port_index);
		ERR_FAIL_COND(err != OK);
		ERR_FAIL_COND(con.dst.port_index >= to_node_view->input_controls.size());
		Control *input_control = to_node_view->input_controls[con.dst.port_index];
		ERR_FAIL_COND(input_control == nullptr);
		input_control->hide();
	}
}

Control *VoxelGraphEditor::create_param_control(uint32_t node_id, Variant value, const VoxelGraphNodeDB::Param &param, bool is_input) {
	Control *param_control = nullptr;

	switch (param.type) {
		case Variant::REAL: {
			SpinBox *spinbox = memnew(SpinBox);
			spinbox->set_step(0.001);
			spinbox->set_min(-10000.0);
			spinbox->set_max(10000.0);
			spinbox->set_value(value);
			spinbox->connect("value_changed", this, "_on_node_param_spinbox_value_changed", varray(node_id, param.index, is_input));
			param_control = spinbox;
		} break;

		case Variant::OBJECT: {
			Label *placeholder = memnew(Label);
			placeholder->set_text("<Resource>");
			param_control = placeholder;
		} break;

		default:
			CRASH_NOW();
			break;
	}

	return param_control;
}

void VoxelGraphEditor::create_node_gui(uint32_t node_id) {
	// Build one GUI node

	CRASH_COND(_graph.is_null());
	const VoxelGeneratorGraph &graph = **_graph;
	const uint32_t node_type_id = graph.get_node_type_id(node_id);
	const VoxelGraphNodeDB::NodeType &node_type = VoxelGraphNodeDB::get_singleton()->get_type(node_type_id);

	const String node_name = node_to_gui_name(node_id);
	ERR_FAIL_COND(_graph_edit->has_node(node_name));

	VoxelGraphEditorNode *node_view = memnew(VoxelGraphEditorNode);
	node_view->set_offset(graph.get_node_gui_position(node_id) * EDSCALE);
	node_view->set_title(node_type.name);
	node_view->set_name(node_name);
	node_view->node_id = node_id;
	node_view->connect("dragged", this, "_on_graph_node_dragged", varray(node_id));
	//node_view.resizable = true
	//node_view.rect_size = Vector2(200, 100)

	const int row_count = max(node_type.inputs.size(), node_type.outputs.size()) + node_type.params.size();
	const Color port_color(0.4, 0.4, 1.0);
	uint32_t param_index = 0;

	for (int i = 0; i < row_count; ++i) {
		const bool has_left = i < node_type.inputs.size();
		const bool has_right = i < node_type.outputs.size();

		HBoxContainer *property_control = memnew(HBoxContainer);
		property_control->set_custom_minimum_size(Vector2(0, 24 * EDSCALE));

		if (has_left) {
			Label *label = memnew(Label);
			label->set_text(node_type.inputs[i].name);
			property_control->add_child(label);

			Variant defval = graph.get_node_default_input(node_id, i);
			VoxelGraphNodeDB::Param param("", defval.get_type());
			param.index = i;
			Control *control = create_param_control(node_id, defval, param, true);

			control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			property_control->add_child(control);
			CRASH_COND(i != node_view->input_controls.size());
			node_view->input_controls.push_back(control);
		}

		if (has_right) {
			if (property_control->get_child_count() < 2) {
				Control *spacer = memnew(Control);
				spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				property_control->add_child(spacer);
			}

			Label *label = memnew(Label);
			label->set_text(node_type.outputs[i].name);
			property_control->add_child(label);
		}

		if (!has_left && !has_right) {
			const VoxelGraphNodeDB::Param &param = node_type.params[param_index];

			Label *label = memnew(Label);
			label->set_text(param.name);
			property_control->add_child(label);

			Variant param_value = graph.get_node_param(node_id, param_index);
			Control *param_control = create_param_control(node_id, param_value, param, false);

			param_control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			property_control->add_child(param_control);
			CRASH_COND(param_index != node_view->param_controls.size());
			node_view->param_controls.push_back(param_control);

			++param_index;
		}

		node_view->add_child(property_control);
		node_view->set_slot(i, has_left, Variant::REAL, port_color, has_right, Variant::REAL, port_color);
	}

	_graph_edit->add_child(node_view);
}

void VoxelGraphEditor::_on_node_param_spinbox_value_changed(float value, int node_id, int param_index, bool is_input) {
	if (_updating_param_gui) {
		// When undoing, editor controls will emit "changed" signals,
		// but we don't want to treat those as actual actions
		return;
	}

	if (is_input) {
		_undo_redo->create_action(TTR("Set Node Default Input"));
		Variant previous_value = _graph->get_node_default_input(node_id, param_index);
		_undo_redo->add_do_method(*_graph, "set_node_default_input", node_id, param_index, value);
		_undo_redo->add_undo_method(*_graph, "set_node_default_input", node_id, param_index, previous_value);

	} else {
		_undo_redo->create_action(TTR("Set Node Param"));
		Variant previous_value = _graph->get_node_param(node_id, param_index);

		_undo_redo->add_do_method(*_graph, "set_node_param", node_id, param_index, value);

		// TODO I had to complicate this because UndoRedo confuses `null` as `absence of argument`, which causes method call errors
		// See https://github.com/godotengine/godot/issues/36895
		if (previous_value.get_type() == Variant::NIL) {
			_undo_redo->add_undo_method(*_graph, "set_node_param_null", node_id, param_index);
		} else {
			_undo_redo->add_undo_method(*_graph, "set_node_param", node_id, param_index, previous_value);
		}
	}

	_undo_redo->add_do_method(this, "update_node_param_gui", node_id, param_index, is_input);
	_undo_redo->add_undo_method(this, "update_node_param_gui", node_id, param_index, is_input);
	_undo_redo->commit_action();
}

void VoxelGraphEditor::update_node_param_gui(int node_id, int param_index, bool is_default_input) {
	VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_node(node_to_gui_name(node_id)));

	Variant value;
	Variant::Type value_type;
	Control *param_control;

	if (is_default_input) {
		value_type = Variant::REAL;
		value = _graph->get_node_default_input(node_id, param_index);
		CRASH_COND(param_index >= node_view->input_controls.size());
		param_control = node_view->input_controls[param_index];

	} else {
		const uint32_t node_type_id = _graph->get_node_type_id(node_id);
		const VoxelGraphNodeDB::NodeType &node_type = VoxelGraphNodeDB::get_singleton()->get_type(node_type_id);
		CRASH_COND(param_index >= node_type.params.size());
		const VoxelGraphNodeDB::Param &param = node_type.params[param_index];
		value_type = param.type;
		value = _graph->get_node_param(node_id, param_index);
		CRASH_COND(param_index >= node_view->param_controls.size());
		param_control = node_view->param_controls[param_index];
	}

	_updating_param_gui = true;

	switch (value_type) {
		case Variant::REAL: {
			SpinBox *spinbox = Object::cast_to<SpinBox>(param_control);
			ERR_FAIL_COND(spinbox == nullptr);
			spinbox->set_value(value);
		} break;

		case Variant::OBJECT: {
			// TODO
		} break;

		default:
			CRASH_NOW();
			break;
	}

	_updating_param_gui = false;
}

void remove_connections_from_and_to(GraphEdit *graph_edit, StringName node_name) {
	// Get copy of connection list
	List<GraphEdit::Connection> connections;
	graph_edit->get_connection_list(&connections);

	for (List<GraphEdit::Connection>::Element *E = connections.front(); E; E = E->next()) {
		const GraphEdit::Connection &con = E->get();
		if (con.from == node_name || con.to == node_name) {
			graph_edit->disconnect_node(con.from, con.from_port, con.to, con.to_port);
		}
	}
}

static NodePath to_node_path(StringName sn) {
	Vector<StringName> path;
	path.push_back(sn);
	return NodePath(path, false);
}

void VoxelGraphEditor::remove_node_gui(StringName gui_node_name) {
	// Remove connections from the UI, because GraphNode doesn't do it...
	remove_connections_from_and_to(_graph_edit, gui_node_name);
	Node *node_view = _graph_edit->get_node(to_node_path(gui_node_name));
	ERR_FAIL_COND(Object::cast_to<GraphNode>(node_view) == nullptr);
	memdelete(node_view);
}

void VoxelGraphEditor::_on_graph_edit_gui_input(Ref<InputEvent> event) {
	Ref<InputEventMouseButton> mb = event;

	if (mb.is_valid()) {
		if (mb->is_pressed()) {
			if (mb->get_button_index() == BUTTON_RIGHT) {
				_click_position = mb->get_position();
				_context_menu->set_position(get_global_mouse_position());
				_context_menu->popup();
			}
		}
	}
}

void VoxelGraphEditor::_on_graph_edit_connection_request(String from_node_name, int from_slot, String to_node_name, int to_slot) {
	VoxelGraphEditorNode *src_node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_node(from_node_name));
	VoxelGraphEditorNode *dst_node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_node(to_node_name));
	ERR_FAIL_COND(src_node_view == nullptr);
	ERR_FAIL_COND(dst_node_view == nullptr);
	//print("Connection attempt from ", from, ":", from_slot, " to ", to, ":", to_slot)
	if (_graph->can_connect(src_node_view->node_id, from_slot, dst_node_view->node_id, to_slot)) {
		_undo_redo->create_action(TTR("Connect Nodes"));

		_undo_redo->add_do_method(*_graph, "add_connection", src_node_view->node_id, from_slot, dst_node_view->node_id, to_slot);
		_undo_redo->add_do_method(_graph_edit, "connect_node", from_node_name, from_slot, to_node_name, to_slot);
		_undo_redo->add_do_method(this, "set_input_control_visible", to_node_name, to_slot, false);

		_undo_redo->add_undo_method(*_graph, "remove_connection", src_node_view->node_id, from_slot, dst_node_view->node_id, to_slot);
		_undo_redo->add_undo_method(_graph_edit, "disconnect_node", from_node_name, from_slot, to_node_name, to_slot);
		_undo_redo->add_undo_method(this, "set_input_control_visible", to_node_name, to_slot, true);

		_undo_redo->commit_action();
	}
}

void VoxelGraphEditor::_on_graph_edit_disconnection_request(String from_node_name, int from_slot, String to_node_name, int to_slot) {
	VoxelGraphEditorNode *src_node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_node(from_node_name));
	VoxelGraphEditorNode *dst_node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_node(to_node_name));
	ERR_FAIL_COND(src_node_view == nullptr);
	ERR_FAIL_COND(dst_node_view == nullptr);

	_undo_redo->create_action(TTR("Disconnect Nodes"));

	_undo_redo->add_do_method(*_graph, "remove_connection", src_node_view->node_id, from_slot, dst_node_view->node_id, to_slot);
	_undo_redo->add_do_method(_graph_edit, "disconnect_node", from_node_name, from_slot, to_node_name, to_slot);
	_undo_redo->add_do_method(this, "set_input_control_visible", to_node_name, to_slot, true);

	_undo_redo->add_undo_method(*_graph, "add_connection", src_node_view->node_id, from_slot, dst_node_view->node_id, to_slot);
	_undo_redo->add_undo_method(_graph_edit, "connect_node", from_node_name, from_slot, to_node_name, to_slot);
	_undo_redo->add_undo_method(this, "set_input_control_visible", to_node_name, to_slot, false);

	_undo_redo->commit_action();
}

void VoxelGraphEditor::set_input_control_visible(String node_name, int input_index, bool visible) {
	VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_node(node_name));
	ERR_FAIL_COND(node_view == nullptr);
	ERR_FAIL_INDEX(input_index, node_view->input_controls.size());
	node_view->input_controls[input_index]->set_visible(visible);
}

void VoxelGraphEditor::_on_graph_edit_delete_nodes_request() {

	std::vector<VoxelGraphEditorNode *> to_erase;

	for (int i = 0; i < _graph_edit->get_child_count(); ++i) {
		VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_child(i));
		if (node_view != nullptr) {
			if (node_view->is_selected()) {
				to_erase.push_back(node_view);
			}
		}
	}

	_undo_redo->create_action(TTR("Delete Nodes"));

	std::vector<ProgramGraph::Connection> connections;
	_graph->get_connections(connections);

	for (size_t i = 0; i < to_erase.size(); ++i) {
		const VoxelGraphEditorNode *node_view = to_erase[i];
		const uint32_t node_id = node_view->node_id;
		const uint32_t node_type_id = _graph->get_node_type_id(node_id);

		_undo_redo->add_do_method(*_graph, "remove_node", node_id);
		_undo_redo->add_do_method(this, "remove_node_gui", node_view->get_name());

		_undo_redo->add_undo_method(*_graph, "create_node", node_type_id, _graph->get_node_gui_position(node_id), node_id);

		// Params undo
		const size_t param_count = VoxelGraphNodeDB::get_singleton()->get_type(node_type_id).params.size();
		for (size_t j = 0; j < param_count; ++j) {
			Variant param_value = _graph->get_node_param(node_id, j);
			_undo_redo->add_undo_method(*_graph, "set_node_param", node_id, j, param_value);
		}

		_undo_redo->add_undo_method(this, "create_node_gui", node_id);

		// Connections undo
		for (size_t j = 0; j < connections.size(); ++j) {
			const ProgramGraph::Connection &con = connections[j];
			if (con.src.node_id == node_id || con.dst.node_id == node_id) {
				_undo_redo->add_undo_method(*_graph, "add_connection", con.src.node_id, con.src.port_index, con.dst.node_id, con.dst.port_index);
				String src_node_name = node_to_gui_name(con.src.node_id);
				String dst_node_name = node_to_gui_name(con.dst.node_id);
				_undo_redo->add_undo_method(_graph_edit, "connect_node", src_node_name, con.src.port_index, dst_node_name, con.dst.port_index);
			}
		}
	}

	_undo_redo->commit_action();
}

void VoxelGraphEditor::_on_graph_node_dragged(Vector2 from, Vector2 to, int id) {
	_undo_redo->create_action(TTR("Move nodes"));
	_undo_redo->add_do_method(this, "set_node_position", id, to);
	_undo_redo->add_undo_method(this, "set_node_position", id, from);
	_undo_redo->commit_action();
	// I haven't the faintest idea how VisualScriptEditor magically makes this work neither using `create_action` nor `commit_action`.
}

void VoxelGraphEditor::set_node_position(int id, Vector2 offset) {
	String node_name = node_to_gui_name(id);
	GraphNode *node_view = Object::cast_to<GraphNode>(_graph_edit->get_node(node_name));
	if (node_view != nullptr) {
		node_view->set_offset(offset);
	}
	_graph->set_node_gui_position(id, offset / EDSCALE);
}

Vector2 get_graph_offset_from_mouse(const GraphEdit *graph_edit, const Vector2 local_mouse_pos) {
	// TODO Ask for a method, or at least documentation about how it's done
	Vector2 offset = graph_edit->get_scroll_ofs() + local_mouse_pos;
	if (graph_edit->is_using_snap()) {
		const int snap = graph_edit->get_snap();
		offset = offset.snapped(Vector2(snap, snap));
	}
	offset /= EDSCALE;
	offset /= graph_edit->get_zoom();
	return offset;
}

void VoxelGraphEditor::_on_context_menu_index_pressed(int idx) {
	const Vector2 pos = get_graph_offset_from_mouse(_graph_edit, _click_position);
	const uint32_t node_type_id = _context_menu->get_item_metadata(idx);
	const uint32_t node_id = _graph->generate_node_id();
	const StringName node_name = node_to_gui_name(node_id);

	_undo_redo->create_action(TTR("Create Node"));
	_undo_redo->add_do_method(*_graph, "create_node", node_type_id, pos, node_id);
	_undo_redo->add_do_method(this, "create_node_gui", node_id);
	_undo_redo->add_undo_method(*_graph, "remove_node", node_id);
	_undo_redo->add_undo_method(this, "remove_node_gui", node_name);
	_undo_redo->commit_action();
}

void VoxelGraphEditor::_bind_methods() {

	ClassDB::bind_method(D_METHOD("_on_graph_edit_gui_input", "event"), &VoxelGraphEditor::_on_graph_edit_gui_input);
	ClassDB::bind_method(D_METHOD("_on_graph_edit_connection_request", "from_node_name", "from_slot", "to_node_name", "to_slot"),
			&VoxelGraphEditor::_on_graph_edit_connection_request);
	ClassDB::bind_method(D_METHOD("_on_graph_edit_disconnection_request", "from_node_name", "from_slot", "to_node_name", "to_slot"),
			&VoxelGraphEditor::_on_graph_edit_disconnection_request);
	ClassDB::bind_method(D_METHOD("_on_graph_edit_delete_nodes_request"), &VoxelGraphEditor::_on_graph_edit_delete_nodes_request);
	ClassDB::bind_method(D_METHOD("_on_graph_node_dragged", "from", "to", "id"), &VoxelGraphEditor::_on_graph_node_dragged);
	ClassDB::bind_method(D_METHOD("_on_context_menu_index_pressed", "idx"), &VoxelGraphEditor::_on_context_menu_index_pressed);
	ClassDB::bind_method(D_METHOD("_on_node_param_spinbox_value_changed", "value", "node_id", "param_index", "is_input"),
			&VoxelGraphEditor::_on_node_param_spinbox_value_changed);

	ClassDB::bind_method(D_METHOD("create_node_gui", "node_id"), &VoxelGraphEditor::create_node_gui);
	ClassDB::bind_method(D_METHOD("remove_node_gui", "node_name"), &VoxelGraphEditor::remove_node_gui);
	ClassDB::bind_method(D_METHOD("set_node_position", "node_id", "offset"), &VoxelGraphEditor::set_node_position);
	ClassDB::bind_method(D_METHOD("update_node_param_gui", "node_id", "param_id", "is_default_input"), &VoxelGraphEditor::update_node_param_gui);
	ClassDB::bind_method(D_METHOD("set_input_control_visible", "node_name", "input_index", "visible"), &VoxelGraphEditor::set_input_control_visible);
}
