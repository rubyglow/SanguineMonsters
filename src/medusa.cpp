#include "plugin.hpp"
#include "sanguinecomponents.hpp"
using simd::float_4;

#define MEDUSA_MAX_PORTS 32

struct Medusa : Module {

	enum ParamIds {
		PARAMS_COUNT
	};

	enum InputIds {
		ENUMS(INPUT_VOLTAGE, MEDUSA_MAX_PORTS),
		INPUTS_COUNT
	};

	enum OutputIds {
		ENUMS(OUTPUT_VOLTAGE, MEDUSA_MAX_PORTS),
		OUTPUTS_COUNT
	};

	enum LightIds {
		ENUMS(LIGHT_NORMALLED_PORT, MEDUSA_MAX_PORTS * 3),
		LIGHTS_COUNT
	};

	enum Colors {
		YELLOW,
		RED,
		GREEN,
		BLUE,
		PURPLE
	};

	const int kLightFrequency = 1024;
	int inputsConnected = 0;

	dsp::ClockDivider lightDivider;

	Medusa() {
		config(PARAMS_COUNT, INPUTS_COUNT, OUTPUTS_COUNT, LIGHTS_COUNT);

		for (int i = 0; i < MEDUSA_MAX_PORTS; i++) {
			configInput(INPUT_VOLTAGE + i, string::f("Medusa %d", i + 1));
			configOutput(OUTPUT_VOLTAGE + i, string::f("Medusa %d", i + 1));
		}

		lightDivider.setDivision(kLightFrequency);
	}

	void process(const ProcessArgs& args) override {
		inputsConnected = 0;

		for (int i = 0; i < MEDUSA_MAX_PORTS; i++) {
			if (inputs[INPUT_VOLTAGE + i].isConnected()) {
				inputsConnected++;
			}
		}

		if (inputsConnected > 0) {
			int channelCount = 0;
			int activePort = 0;

			for (int i = 0; i < MEDUSA_MAX_PORTS; i++) {
				if (inputs[INPUT_VOLTAGE + i].isConnected()) {
					channelCount = inputs[INPUT_VOLTAGE + i].getChannels();
					activePort = i;
				}

				if (outputs[OUTPUT_VOLTAGE + i].isConnected()) {
					outputs[i].setChannels(channelCount);

					for (int channel = 0; channel < channelCount; channel += 4) {
						float_4 voltages = inputs[activePort].getVoltageSimd<float_4>(channel);
						outputs[OUTPUT_VOLTAGE + i].setVoltageSimd(voltages, channel);
					}
				}
			}
		}

		if (lightDivider.process()) {
			const float sampleTime = kLightFrequency * args.sampleTime;
			float voltageSum = 0.f;
			Colors ledColor = RED;

			if (inputsConnected > 0) {
				for (int i = 0; i < MEDUSA_MAX_PORTS; i++) {
					if (inputs[INPUT_VOLTAGE + i].isConnected()) {
						voltageSum = clamp(inputs[INPUT_VOLTAGE + i].getVoltageSum(), -10.f, 10.f);
					}

					if (voltageSum <= -7) {
						ledColor = YELLOW;
					}
					else if (voltageSum >= -6.99 && voltageSum <= -3) {
						ledColor = RED;
					}
					else if (voltageSum >= -2.99 && voltageSum <= 1) {
						ledColor = GREEN;
					}
					else if (voltageSum > 1 && voltageSum <= 5.99) {
						ledColor = BLUE;
					}
					else {
						ledColor = PURPLE;
					}

					int currentLight = LIGHT_NORMALLED_PORT + i * 3;

					float rescaledValue;
					switch (ledColor)
					{
					case YELLOW:
					{
						rescaledValue = math::rescale(voltageSum, -10, -7, 0.1f, 1.f);
						lights[currentLight + 0].setBrightnessSmooth(rescaledValue, sampleTime);
						lights[currentLight + 1].setBrightnessSmooth(rescaledValue, sampleTime);
						lights[currentLight + 2].setBrightnessSmooth(0.f, sampleTime);
						break;
					}
					case RED:
					{
						lights[currentLight + 0].setBrightnessSmooth(math::rescale(voltageSum, -6.99, -3, 0.1f, 1.f), sampleTime);
						lights[currentLight + 1].setBrightnessSmooth(0.f, sampleTime);
						lights[currentLight + 2].setBrightnessSmooth(0.f, sampleTime);
						break;
					}
					case GREEN: {
						lights[currentLight + 0].setBrightnessSmooth(0.f, sampleTime);
						lights[currentLight + 1].setBrightnessSmooth(math::rescale(voltageSum, -2.99, 1, 0.1f, 1.f), sampleTime);
						lights[currentLight + 2].setBrightnessSmooth(0.f, sampleTime);
						break;
					}
					case BLUE: {
						lights[currentLight + 0].setBrightnessSmooth(0.f, sampleTime);
						lights[currentLight + 1].setBrightnessSmooth(0.f, sampleTime);
						lights[currentLight + 2].setBrightnessSmooth(math::rescale(voltageSum, 1.01, 5.99, 0.1f, 1.f), sampleTime);
						break;
					}
					case PURPLE:
					{
						rescaledValue = math::rescale(voltageSum, 6, 10, 0.1f, 1.f);
						lights[currentLight + 0].setBrightnessSmooth(rescaledValue, sampleTime);
						lights[currentLight + 1].setBrightnessSmooth(0.f, sampleTime);
						lights[currentLight + 2].setBrightnessSmooth(rescaledValue, sampleTime);
						break;
					}
					}
				}
			}
			else {
				for (int i = 0; i < MEDUSA_MAX_PORTS; i++) {
					int currentLight = LIGHT_NORMALLED_PORT + i * 3;

					lights[currentLight + 0].setBrightnessSmooth(0.f, sampleTime);
					lights[currentLight + 1].setBrightnessSmooth(0.f, sampleTime);
					lights[currentLight + 2].setBrightnessSmooth(0.f, sampleTime);
				}
			}
		}
	}
};

struct MedusaWidget : ModuleWidget {
	MedusaWidget(Medusa* module) {
		setModule(module);
		setPanel(Svg::load(asset::plugin(pluginInstance, "res/medusa.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		SanguinePolyInputLight* lightInput1 = new SanguinePolyInputLight;
		lightInput1->box.pos = mm2px(Vec(4.825, 20.098));
		lightInput1->module = module;
		addChild(lightInput1);

		SanguinePolyOutputLight* lightOutput1 = new SanguinePolyOutputLight;
		lightOutput1->box.pos = mm2px(Vec(21.331, 20.098));
		lightOutput1->module = module;
		addChild(lightOutput1);

		float xInputs = 8.119;
		float xOutputs = 24.625;
		float xLights = 16.378;

		float yPortBase = 29.326;
		const float yDelta = 9.827;

		float yLightBase = 29.326;

		float currentPortY = yPortBase;
		float currentLightY = yLightBase;

		int portOffset = 0;

		for (int i = 0; i < 10; i++) {
			addInput(createInputCentered<BananutGreen>(mm2px(Vec(xInputs, currentPortY)), module, Medusa::INPUT_VOLTAGE + portOffset + i));
			addOutput(createOutputCentered<BananutRed>(mm2px(Vec(xOutputs, currentPortY)), module, Medusa::OUTPUT_VOLTAGE + portOffset + i));

			addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(xLights, currentLightY)), module, (Medusa::LIGHT_NORMALLED_PORT + i + portOffset) * 3));

			currentPortY += yDelta;
			currentLightY += yDelta;
		}

		SanguinePolyInputLight* lightInput2 = new SanguinePolyInputLight;
		lightInput2->box.pos = mm2px(Vec(39.618, 20.098));
		lightInput2->module = module;
		addChild(lightInput2);

		SanguinePolyOutputLight* lightOutput2 = new SanguinePolyOutputLight;
		lightOutput2->box.pos = mm2px(Vec(56.124, 20.098));
		lightOutput2->module = module;
		addChild(lightOutput2);

		xInputs = 42.912;
		xOutputs = 59.418;
		xLights = 51.171;

		currentPortY = yPortBase;
		currentLightY = yLightBase;

		portOffset = 10;

		for (int i = 0; i < 6; i++) {
			addInput(createInputCentered<BananutGreen>(mm2px(Vec(xInputs, currentPortY)), module, Medusa::INPUT_VOLTAGE + portOffset + i));
			addOutput(createOutputCentered<BananutRed>(mm2px(Vec(xOutputs, currentPortY)), module, Medusa::OUTPUT_VOLTAGE + portOffset + i));

			addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(xLights, currentLightY)), module, (Medusa::LIGHT_NORMALLED_PORT + i + portOffset) * 3));

			currentPortY += yDelta;
			currentLightY += yDelta;
		}

		SanguinePolyInputLight* lightInput3 = new SanguinePolyInputLight;
		lightInput3->box.pos = mm2px(Vec(74.448, 20.098));
		lightInput3->module = module;
		addChild(lightInput3);

		SanguinePolyOutputLight* lightOutput3 = new SanguinePolyOutputLight;
		lightOutput3->box.pos = mm2px(Vec(90.954, 20.098));
		lightOutput3->module = module;
		addChild(lightOutput3);

		xInputs = 77.742;
		xOutputs = 94.248;
		xLights = 86.001;

		currentPortY = yPortBase;
		currentLightY = yLightBase;

		portOffset = 16;

		for (int i = 0; i < 6; i++) {
			addInput(createInputCentered<BananutGreen>(mm2px(Vec(xInputs, currentPortY)), module, Medusa::INPUT_VOLTAGE + portOffset + i));
			addOutput(createOutputCentered<BananutRed>(mm2px(Vec(xOutputs, currentPortY)), module, Medusa::OUTPUT_VOLTAGE + portOffset + i));

			addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(xLights, currentLightY)), module, (Medusa::LIGHT_NORMALLED_PORT + i + portOffset) * 3));

			currentPortY += yDelta;
			currentLightY += yDelta;
		}

		SanguinePolyInputLight* lightInput4 = new SanguinePolyInputLight;
		lightInput4->box.pos = mm2px(Vec(109.241, 20.098));
		lightInput4->module = module;
		addChild(lightInput4);

		SanguinePolyOutputLight* lightOutput4 = new SanguinePolyOutputLight;
		lightOutput4->box.pos = mm2px(Vec(125.747, 20.098));
		lightOutput4->module = module;
		addChild(lightOutput4);

		xInputs = 112.535;
		xOutputs = 129.041;
		xLights = 120.794;

		currentPortY = yPortBase;
		currentLightY = yLightBase;

		portOffset = 22;

		for (int i = 0; i < 10; i++) {
			addInput(createInputCentered<BananutGreen>(mm2px(Vec(xInputs, currentPortY)), module, Medusa::INPUT_VOLTAGE + portOffset + i));
			addOutput(createOutputCentered<BananutRed>(mm2px(Vec(xOutputs, currentPortY)), module, Medusa::OUTPUT_VOLTAGE + portOffset + i));

			addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(xLights, currentLightY)), module, (Medusa::LIGHT_NORMALLED_PORT + i + portOffset) * 3));

			currentPortY += yDelta;
			currentLightY += yDelta;
		}

		SanguineShapedLight* bloodLight = new SanguineShapedLight();
		bloodLight->box.pos = mm2px(Vec(56.919, 106.265));
		bloodLight->module = module;
		bloodLight->setSvg(Svg::load(asset::plugin(pluginInstance, "res/blood_glowy.svg")));
		addChild(bloodLight);

		SanguineShapedLight* monstersLight = new SanguineShapedLight();
		monstersLight->box.pos = mm2px(Vec(64.24, 114.231));
		monstersLight->module = module;
		monstersLight->setSvg(Svg::load(asset::plugin(pluginInstance, "res/monsters_lit.svg")));
		addChild(monstersLight);
	}
};

Model* modelMedusa = createModel<Medusa, MedusaWidget>("Sanguine-Medusa");