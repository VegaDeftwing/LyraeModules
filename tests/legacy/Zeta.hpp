#include "plugin.hpp"

struct Zeta : Module {
  enum ParamIds { FLIPPED_PARAM, ALT_PARAM, NUM_PARAMS };
  enum InputIds { NUM_INPUTS };
  enum OutputIds { NUM_OUTPUTS };
  enum LightIds { NUM_LIGHTS };

  Zeta() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(ALT_PARAM, 0.f, 2.f, 0.f, "Alt Display");
  }

  void process(const ProcessArgs &args) override {}
};

// struct SomeWidgetBuffer : FramebufferWidget {
//   Zeta *module;
//   SomeWidgetBuffer(Zeta *m) { module = m; }
//   void step() override {
//     // if(module->dirty){
//     //   FramebufferWidget::dirty = true;
//     //   module->dirty = false;
//     // }

//     FramebufferWidget::step();
//   }
// };

// struct BlankWidget : Widget {
//   Zeta *module;
//   BlankWidget(Zeta *m) {
//     module = m;
//     box.pos = Vec(0, 0);
//     box.size = Vec(100, 1000);
//   }
//   void draw(NVGcontext *vg) override {
//     /*
//     VCV-Vortico: call Widget::draw() in drawLayer(args, 1)
//     and don't call the super method in draw().
//     Then any children will be drawn on layer 1.
//     Remember to use a FramebufferWidget to avoiding redrawing your SVG
//     every frame.
//     */

//     std::shared_ptr<Svg> svg =
//         APP->window->loadSvg(asset::plugin(pluginInstance, "res/Zeta.svg"));

//     if (module != NULL) {
//       Zeta *zeta = dynamic_cast<Zeta *>(module);

//       if (zeta->paramQuantities[Zeta::ALT_PARAM]->getValue() == 1.f) {
//         if (zeta->paramQuantities[Zeta::FLIPPED_PARAM]->getValue() == 1.f) {
//           svg = APP->window->loadSvg(
//               asset::plugin(pluginInstance, "res/Zetab.svg"));
//         } else {
//           svg = APP->window->loadSvg(
//               asset::plugin(pluginInstance, "res/ZetabAlt.svg"));
//         }

//       } else if (zeta->paramQuantities[Zeta::ALT_PARAM]->getValue() == 2.f) {
//         if (zeta->paramQuantities[Zeta::FLIPPED_PARAM]->getValue() == 1.f) {
//           svg = APP->window->loadSvg(
//               asset::plugin(pluginInstance, "res/Zetac.svg"));
//         } else {
//           svg = APP->window->loadSvg(
//               asset::plugin(pluginInstance, "res/ZetacAlt.svg"));
//         }
//       } else {
//         if (zeta->paramQuantities[Zeta::FLIPPED_PARAM]->getValue() == 1.f) {
//           svg = APP->window->loadSvg(
//               asset::plugin(pluginInstance, "res/Zeta.svg"));
//         } else {
//           svg = APP->window->loadSvg(
//               asset::plugin(pluginInstance, "res/ZetaAlt.svg"));
//         }
//       }
//     }

//     if (svg && svg->handle) {
//       svgDraw(vg, svg->handle);
//     }
//   }
// };
