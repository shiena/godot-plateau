#include "plateau_grid_code.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

PLATEAUGridCode::PLATEAUGridCode() {
}

PLATEAUGridCode::~PLATEAUGridCode() {
}

Ref<PLATEAUGridCode> PLATEAUGridCode::parse(const String &code) {
    Ref<PLATEAUGridCode> result;
    result.instantiate();

    try {
        result->grid_code_ = plateau::dataset::GridCode::create(code.utf8().get_data());
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUGridCode: Failed to parse '", code, "': ", e.what());
    }

    return result;
}

String PLATEAUGridCode::get_code() const {
    if (!grid_code_ || !grid_code_->isValid()) {
        return String();
    }
    return String::utf8(grid_code_->get().c_str());
}

Dictionary PLATEAUGridCode::get_extent() const {
    Dictionary result;

    if (!grid_code_ || !grid_code_->isValid()) {
        return result;
    }

    try {
        auto extent = grid_code_->getExtent();
        result["min_lat"] = extent.min.latitude;
        result["max_lat"] = extent.max.latitude;
        result["min_lon"] = extent.min.longitude;
        result["max_lon"] = extent.max.longitude;
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUGridCode: Failed to get extent: ", e.what());
    }

    return result;
}

bool PLATEAUGridCode::is_valid() const {
    return grid_code_ && grid_code_->isValid();
}

Ref<PLATEAUGridCode> PLATEAUGridCode::upper() const {
    Ref<PLATEAUGridCode> result;
    result.instantiate();

    if (!grid_code_ || !grid_code_->isValid()) {
        return result;
    }

    try {
        result->grid_code_ = grid_code_->upper();
    } catch (const std::exception &e) {
        UtilityFunctions::printerr("PLATEAUGridCode: Failed to get upper level: ", e.what());
    }

    return result;
}

int PLATEAUGridCode::get_level() const {
    if (!grid_code_ || !grid_code_->isValid()) {
        return -1;
    }
    return grid_code_->getLevel();
}

bool PLATEAUGridCode::is_largest_level() const {
    if (!grid_code_ || !grid_code_->isValid()) {
        return false;
    }
    return grid_code_->isLargestLevel();
}

bool PLATEAUGridCode::is_smaller_than_normal_gml() const {
    if (!grid_code_ || !grid_code_->isValid()) {
        return false;
    }
    return grid_code_->isSmallerThanNormalGml();
}

bool PLATEAUGridCode::is_normal_gml_level() const {
    if (!grid_code_ || !grid_code_->isValid()) {
        return false;
    }
    return grid_code_->isNormalGmlLevel();
}

void PLATEAUGridCode::set_native(const std::shared_ptr<plateau::dataset::GridCode> &grid_code) {
    grid_code_ = grid_code;
}

void PLATEAUGridCode::_bind_methods() {
    ClassDB::bind_static_method("PLATEAUGridCode", D_METHOD("parse", "code"), &PLATEAUGridCode::parse);

    ClassDB::bind_method(D_METHOD("get_code"), &PLATEAUGridCode::get_code);
    ClassDB::bind_method(D_METHOD("get_extent"), &PLATEAUGridCode::get_extent);
    ClassDB::bind_method(D_METHOD("is_valid"), &PLATEAUGridCode::is_valid);
    ClassDB::bind_method(D_METHOD("upper"), &PLATEAUGridCode::upper);
    ClassDB::bind_method(D_METHOD("get_level"), &PLATEAUGridCode::get_level);
    ClassDB::bind_method(D_METHOD("is_largest_level"), &PLATEAUGridCode::is_largest_level);
    ClassDB::bind_method(D_METHOD("is_smaller_than_normal_gml"), &PLATEAUGridCode::is_smaller_than_normal_gml);
    ClassDB::bind_method(D_METHOD("is_normal_gml_level"), &PLATEAUGridCode::is_normal_gml_level);
}
