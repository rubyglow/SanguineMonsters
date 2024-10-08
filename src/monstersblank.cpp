#include "plugin.hpp"
#include "sanguinecomponents.hpp"
#include "sanguinehelpers.hpp"

struct MonstersBlank : Module {

};

struct MonstersBlankWidget : ModuleWidget {
	MonstersBlankWidget(MonstersBlank* module) {
		setModule(module);

		SanguinePanel* panel = new SanguinePanel("res/backplate_10hp_purple.svg", "res/monsters_blank.svg");
		setPanel(panel);

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		SanguineShapedLight* monstersLight = new SanguineShapedLight(module, "res/monsters_lit_blank.svg", 25.4, 51.62);
		addChild(monstersLight);

		SanguineShapedLight* bloodLight = new SanguineShapedLight(module, "res/blood_glowy_blank.svg", 7.907, 114.709);
		addChild(bloodLight);

		SanguineShapedLight* sanguineLogo = new SanguineShapedLight(module, "res/sanguine_lit_blank.svg", 29.204, 113.209);
		addChild(sanguineLogo);
	}
};

Model* modelMonstersBlank = createModel<MonstersBlank, MonstersBlankWidget>("Sanguine-Monsters-Blank");
