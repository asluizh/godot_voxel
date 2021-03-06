#include "voxel_graph_node_db.h"

namespace {
VoxelGraphNodeDB *g_node_type_db = nullptr;
}

VoxelGraphNodeDB *VoxelGraphNodeDB::get_singleton() {
	CRASH_COND(g_node_type_db == nullptr);
	return g_node_type_db;
}

void VoxelGraphNodeDB::create_singleton() {
	CRASH_COND(g_node_type_db != nullptr);
	g_node_type_db = memnew(VoxelGraphNodeDB());
}

void VoxelGraphNodeDB::destroy_singleton() {
	CRASH_COND(g_node_type_db == nullptr);
	memdelete(g_node_type_db);
	g_node_type_db = nullptr;
}

VoxelGraphNodeDB::VoxelGraphNodeDB() {
	FixedArray<NodeType, VoxelGeneratorGraph::NODE_TYPE_COUNT> &types = _types;

	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_CONSTANT];
		t.name = "Constant";
		t.outputs.push_back(Port("value"));
		t.params.push_back(Param("value", Variant::REAL));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_INPUT_X];
		t.name = "InputX";
		t.outputs.push_back(Port("x"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_INPUT_Y];
		t.name = "InputY";
		t.outputs.push_back(Port("y"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_INPUT_Z];
		t.name = "InputZ";
		t.outputs.push_back(Port("z"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_OUTPUT_SDF];
		t.name = "OutputSDF";
		t.inputs.push_back(Port("sdf"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_ADD];
		t.name = "Add";
		t.inputs.push_back(Port("a"));
		t.inputs.push_back(Port("b"));
		t.outputs.push_back(Port("out"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_SUBTRACT];
		t.name = "Subtract";
		t.inputs.push_back(Port("a"));
		t.inputs.push_back(Port("b"));
		t.outputs.push_back(Port("out"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_MULTIPLY];
		t.name = "Multiply";
		t.inputs.push_back(Port("a"));
		t.inputs.push_back(Port("b"));
		t.outputs.push_back(Port("product"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_SINE];
		t.name = "Sine";
		t.inputs.push_back(Port("x"));
		t.outputs.push_back(Port("out"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_FLOOR];
		t.name = "Floor";
		t.inputs.push_back(Port("x"));
		t.outputs.push_back(Port("out"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_ABS];
		t.name = "Abs";
		t.inputs.push_back(Port("x"));
		t.outputs.push_back(Port("out"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_SQRT];
		t.name = "Sqrt";
		t.inputs.push_back(Port("x"));
		t.outputs.push_back(Port("out"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_DISTANCE_2D];
		t.name = "Distance2D";
		t.inputs.push_back(Port("x0"));
		t.inputs.push_back(Port("y0"));
		t.inputs.push_back(Port("x1"));
		t.inputs.push_back(Port("y1"));
		t.outputs.push_back(Port("out"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_DISTANCE_3D];
		t.name = "Distance3D";
		t.inputs.push_back(Port("x0"));
		t.inputs.push_back(Port("y0"));
		t.inputs.push_back(Port("z0"));
		t.inputs.push_back(Port("x1"));
		t.inputs.push_back(Port("y1"));
		t.inputs.push_back(Port("z1"));
		t.outputs.push_back(Port("out"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_CLAMP];
		t.name = "Clamp";
		t.inputs.push_back(Port("x"));
		t.outputs.push_back(Port("out"));
		t.params.push_back(Param("min", Variant::REAL, -1.f));
		t.params.push_back(Param("max", Variant::REAL, 1.f));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_MIX];
		t.name = "Mix";
		t.inputs.push_back(Port("a"));
		t.inputs.push_back(Port("b"));
		t.inputs.push_back(Port("ratio"));
		t.outputs.push_back(Port("out"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_REMAP];
		t.name = "Remap";
		t.inputs.push_back(Port("x"));
		t.outputs.push_back(Port("out"));
		t.params.push_back(Param("min0", Variant::REAL, -1.f));
		t.params.push_back(Param("max0", Variant::REAL, 1.f));
		t.params.push_back(Param("min1", Variant::REAL, -1.f));
		t.params.push_back(Param("max1", Variant::REAL, 1.f));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_CURVE];
		t.name = "Curve";
		t.inputs.push_back(Port("x"));
		t.outputs.push_back(Port("out"));
		t.params.push_back(Param("curve", "Curve"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_NOISE_2D];
		t.name = "Noise2D";
		t.inputs.push_back(Port("x"));
		t.inputs.push_back(Port("y"));
		t.outputs.push_back(Port("out"));
		t.params.push_back(Param("noise", "OpenSimplexNoise"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_NOISE_3D];
		t.name = "Noise3D";
		t.inputs.push_back(Port("x"));
		t.inputs.push_back(Port("y"));
		t.inputs.push_back(Port("z"));
		t.outputs.push_back(Port("out"));
		t.params.push_back(Param("noise", "OpenSimplexNoise"));
	}
	{
		NodeType &t = types[VoxelGeneratorGraph::NODE_IMAGE_2D];
		t.name = "Image";
		t.inputs.push_back(Port("x"));
		t.inputs.push_back(Port("y"));
		t.outputs.push_back(Port("out"));
		t.params.push_back(Param("image", "Image"));
	}

	for (unsigned int i = 0; i < _types.size(); ++i) {
		NodeType &t = _types[i];
		_type_name_to_id.set(t.name, (VoxelGeneratorGraph::NodeTypeID)i);

		for (size_t param_index = 0; param_index < t.params.size(); ++param_index) {
			Param &p = t.params[param_index];
			t.param_name_to_index.set(p.name, param_index);
			p.index = param_index;

			switch (p.type) {
				case Variant::REAL:
					if (p.default_value.get_type() == Variant::NIL) {
						p.default_value = 0.f;
					}
					break;

				case Variant::OBJECT:
					break;

				default:
					CRASH_NOW();
					break;
			}
		}

		for (size_t input_index = 0; input_index < t.inputs.size(); ++input_index) {
			const Port &p = t.inputs[input_index];
			t.input_name_to_index.set(p.name, input_index);
		}
	}
}

Dictionary VoxelGraphNodeDB::get_type_info_dict(uint32_t id) const {
	const NodeType &type = _types[id];

	Dictionary type_dict;
	type_dict["name"] = type.name;

	Array inputs;
	inputs.resize(type.inputs.size());
	for (size_t i = 0; i < type.inputs.size(); ++i) {
		const Port &input = type.inputs[i];
		Dictionary d;
		d["name"] = input.name;
		inputs[i] = d;
	}

	Array outputs;
	outputs.resize(type.outputs.size());
	for (size_t i = 0; i < type.outputs.size(); ++i) {
		const Port &output = type.outputs[i];
		Dictionary d;
		d["name"] = output.name;
		outputs[i] = d;
	}

	Array params;
	params.resize(type.params.size());
	for (size_t i = 0; i < type.params.size(); ++i) {
		const Param &p = type.params[i];
		Dictionary d;
		d["name"] = p.name;
		d["type"] = p.type;
		d["class_name"] = p.class_name;
		d["default_value"] = p.default_value;
		params[i] = d;
	}

	type_dict["inputs"] = inputs;
	type_dict["outputs"] = outputs;
	type_dict["params"] = params;

	return type_dict;
}

bool VoxelGraphNodeDB::try_get_type_id_from_name(const String &name, VoxelGeneratorGraph::NodeTypeID &out_type_id) const {
	const VoxelGeneratorGraph::NodeTypeID *p = _type_name_to_id.getptr(name);
	if (p == nullptr) {
		return false;
	}
	out_type_id = *p;
	return true;
}

bool VoxelGraphNodeDB::try_get_param_index_from_name(uint32_t type_id, const String &name, uint32_t &out_param_index) const {
	ERR_FAIL_INDEX_V(type_id, _types.size(), false);
	const NodeType &t = _types[type_id];
	const uint32_t *p = t.param_name_to_index.getptr(name);
	if (p == nullptr) {
		return false;
	}
	out_param_index = *p;
	return true;
}

bool VoxelGraphNodeDB::try_get_input_index_from_name(uint32_t type_id, const String &name, uint32_t &out_input_index) const {
	ERR_FAIL_INDEX_V(type_id, _types.size(), false);
	const NodeType &t = _types[type_id];
	const uint32_t *p = t.input_name_to_index.getptr(name);
	if (p == nullptr) {
		return false;
	}
	out_input_index = *p;
	return true;
}
