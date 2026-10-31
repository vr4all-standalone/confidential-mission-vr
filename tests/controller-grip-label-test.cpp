// Exercises the production anchor()/labelPoint() functions, without a GL context.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include "xr_controllers.h"
#include "controller_model.h"

namespace vr::xr::own {
#include "controller-label-names.inc"
}

namespace vr::xr::controllers {
namespace {
constexpr int Left = 0, Right = 1;
const char *const HandName[2] { "left", "right" };
// GPU upload is the boundary: labelPoint only needs to know what draw selected.
struct Model { bool uploaded = true; } own[2];
bool ownLoaded[2] { true, true };
glm::mat4 ownToAim[2] { glm::mat4(1.f), glm::mat4(1.f) };
glm::vec3 ownButton[2][Buttons];
bool ownHasButton[2][Buttons] {};
std::string ownSidecarShown[2] {
    "bundled models/controllers/left.json", "bundled models/controllers/right.json"
};
}
#include "controller-label-production.inc"
}

using namespace vr::xr::controllers;
std::string diagnostic(const LabelPoint& p, int hand, Button button) {
    using namespace vr::xr;
    struct { Button button; } list[] {{button}};
    size_t i = 0;
    std::string result;
#define NOTICE_LOG(category, ...) do { char text[512]; std::snprintf(text,sizeof(text),__VA_ARGS__); result=text; } while (false)
#include "controller-label-diagnostic.inc"
#undef NOTICE_LOG
    return result;
}
int checks = 0, failures = 0;
void check(bool ok, const char *why) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << why << '\n'; }
}
void position(const glm::vec3& got, glm::vec3 want, const char *why) {
    check(glm::length(got - want) < 0.000001f, why);
    if (glm::length(got - want) >= 0.000001f)
        std::cerr << "  got " << got.x << ',' << got.y << ',' << got.z
                  << " want " << want.x << ',' << want.y << ',' << want.z << '\n';
}

int main(int argc, char **argv) {
    // Sidecar aim is 180 degrees around Y. Use exact matrices and a room pose
    // distinct from grip, so using grip or applying modelToAim twice fails.
    glm::mat4 modelToAim(1.f); modelToAim[0][0] = -1.f; modelToAim[2][2] = -1.f;
    ownToAim[0] = ownToAim[1] = modelToAim;
    glm::mat4 aim(1.f); aim[0] = {0,1,0,0}; aim[1] = {-1,0,0,0}; aim[3] = {1,2,3,1};
    const glm::mat4 grip = glm::translate(glm::mat4(1.f), {-3,-4,-5});
    const glm::vec3 wantLocal[] = {{.014388f,-.029244f,.024331f},{-.014388f,-.029244f,.024331f}};
    const glm::vec3 wantRoom[] = {{1.029244f,2.014388f,3.024331f},{1.029244f,1.985612f,3.024331f}};
    for (int hand : {0,1}) {
        auto p = labelPoint(hand, Grip, grip, aim);
        check(p.ownModel && !p.fromFile && p.fromBundled, "missing bundled point uses source fallback, not a sidecar claim");
        position(p.local, wantLocal[hand], "bundled squeeze point follows modelToAim");
        position(p.room, wantRoom[hand], "bundled squeeze line ends on rendered model in room space");
        check(diagnostic(p,hand,Grip).find("in the aim pose (bundled squeeze surface)") != std::string::npos,
              "source fallback log reports its actual aim space and origin");
    }
    // Read the real sidecar grip values supplied by the runner.
    check(argc == 7, "both bundled sidecars contain a three-coordinate grip point");
    if (argc == 7) for (int hand : {0,1}) {
        ownButton[hand][Grip] = {std::stof(argv[1+hand*3]),std::stof(argv[2+hand*3]),std::stof(argv[3+hand*3])};
        ownHasButton[hand][Grip] = true;
        auto p = labelPoint(hand, Grip, grip, aim);
        check(p.ownModel && p.fromFile && !p.fromBundled, "bundled sidecar point is accepted");
        position(p.local, wantLocal[hand], "sidecar point is in baked GLB space");
        position(p.room, wantRoom[hand], "sidecar and durable fallback target the same squeeze surface");
        check(diagnostic(p,hand,Grip).find("in the aim pose (from the file)") != std::string::npos,
              "sidecar point log keeps its actual origin");
    }
    for (int hand : {0,1}) {
        ownHasButton[hand][Grip] = true;
        ownButton[hand][Grip] = {-.006f,-.02f,.01f};
        auto p = labelPoint(hand, Grip, grip, aim);
        check(p.fromFile, "an explicit sidecar point takes precedence");
        position(p.room, {1.02f,2.006f,2.99f}, "an explicit point retains its transform");
        ownHasButton[hand][Grip] = false;
        ownSidecarShown[hand] = "controllers/" + std::string(HandName[hand]) + ".json";
        p = labelPoint(hand, Grip, grip, aim);
        check(p.ownModel && !p.fromFile, "custom model without grip keeps existing fallback");
        position(p.room, hand == 0 ? glm::vec3(-2.985f,-4.006f,-5.008f) : glm::vec3(-3.015f,-4.006f,-5.008f),
                 "custom fallback stays in grip space");
        check(diagnostic(p,hand,Grip).find("in the grip pose (built in: no") != std::string::npos,
              "custom missing point log keeps its grip-space explanation");
        ownLoaded[hand] = false;
        p = labelPoint(hand, Grip, grip, aim);
        check(!p.ownModel && !p.fromFile, "built-in/runtime model does not use bundled own-model points");
        check(diagnostic(p,hand,Grip).find("in the grip pose (built in)") != std::string::npos,
              "built-in/runtime point log keeps its grip-space explanation");
        position(p.room, hand == 0 ? glm::vec3(-2.985f,-4.006f,-5.008f) : glm::vec3(-3.015f,-4.006f,-5.008f),
                 "built-in/runtime fallback is preserved");
        ownLoaded[hand] = true;
        own[hand].uploaded = false;
        ownSidecarShown[hand] = "bundled models/controllers/" + std::string(HandName[hand]) + ".json";
        p = labelPoint(hand, Grip, grip, aim);
        check(!p.ownModel, "failed/not-yet-uploaded own model does not use its grip point");
        own[hand].uploaded = true;
    }
    // Face-button twins and their existing sidecar transformation remain intact.
    ownHasButton[0][X] = true; ownButton[0][X] = {-.0049f,.0046f,-.0097f};
    auto p = labelPoint(0, A, grip, aim);
    check(p.fromFile, "A uses the existing left X twin point");
    position(p.room, {.9954f,2.0049f,3.0097f}, "face-button twins retain the own-model transform");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
