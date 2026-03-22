#include "bamboo/IEngine.h"

#include <cassert>
#include <memory>
#include <string>

namespace {

void test_telex_aw() {
    auto engine = bamboo::api::createEngine("/tmp", "Telex");
    engine->processString("aw");
    assert(engine->getProcessedString() == std::string("ă"));
}

void test_telex_as() {
    auto engine = bamboo::api::createEngine("/tmp", "Telex");
    engine->processString("as");
    assert(engine->getProcessedString() == std::string("á"));
}

void test_telex_dd() {
    auto engine = bamboo::api::createEngine("/tmp", "Telex");
    engine->processString("dd");
    assert(engine->getProcessedString() == std::string("đ"));
}

void test_telex_sawss() {
    auto engine = bamboo::api::createEngine("/tmp", "Telex");
    engine->processString("sawss");
    assert(engine->getProcessedString() == std::string("săs"));
}

void test_telex2_uwow() {
    auto engine = bamboo::api::createEngine("/tmp", "Telex 2");
    engine->processString("uwow");
    assert(engine->getProcessedString() == std::string("ươ"));
}

void test_telex2_thuow_and_backspace() {
    auto engine = bamboo::api::createEngine("/tmp", "Telex 2");
    engine->processString("Thuow");
    assert(engine->getProcessedString() == std::string("Thuơ"));
    engine->removeLastChar(true);
    assert(engine->getProcessedString() == std::string("Thuo"));
}

void test_telex2_choas() {
    auto engine = bamboo::api::createEngine("/tmp", "Telex 2");
    engine->processString("choas");
    assert(engine->getProcessedString() == std::string("choá"));
}

void test_telex_buwoo() {
    auto engine = bamboo::api::createEngine("/tmp", "Telex");
    engine->processString("buwoo");
    assert(engine->getProcessedString() == std::string("buô"));
}

void test_telex_cuongw() {
    auto engine = bamboo::api::createEngine("/tmp", "Telex");
    engine->processString("cuongw");
    assert(engine->getProcessedString() == std::string("cương"));
}

}  // namespace

int main() {
    test_telex_aw();
    test_telex_as();
    test_telex_dd();
    test_telex_sawss();
    test_telex2_uwow();
    test_telex2_thuow_and_backspace();
    test_telex2_choas();
    test_telex_buwoo();
    test_telex_cuongw();
    return 0;
}
