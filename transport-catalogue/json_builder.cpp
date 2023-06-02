#include "json_builder.h"
#include <array>
#include <tuple>
#include <utility>
#include <variant>
#include <algorithm>

namespace json {
    Builder::Builder() {
    }

    DictItemContext &Builder::StartDict() {
        IsReady();
        ExpectKey();
        init = true;
        if (nodes_stack_.back()->IsArray()) {
            auto *ptr = &std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back(Dict{});
            nodes_stack_.emplace_back(ptr);
        } else {
            nodes_stack_.back()->GetValue() = Dict{};
            auto *ptr = nodes_stack_.back();
            if (expect_value) {
                nodes_stack_.pop_back();
                expect_value = false;
            }
            nodes_stack_.emplace_back(ptr);
        }
        return static_cast<DictItemContext &>(*this);
    }

    KeyItemContext &Builder::Key(std::string key) {
        IsReady();
        if (nodes_stack_.back()->IsDict()) {
            auto *ptr = &std::get<Dict>(nodes_stack_.back()->GetValue())[key];
            expect_value = true;
            nodes_stack_.emplace_back(ptr);
        } else {
            throw std::logic_error("Неверная команда");
        }
        return static_cast<KeyItemContext &>(*this);
    }

    Builder &Builder::EndDict() {
        IsReady();
        if (nodes_stack_.back()->IsDict()) {
            nodes_stack_.pop_back();
        } else {
            throw std::logic_error("Неверная команда");
        }
        return *this;
    }

    ArrayItemContext &Builder::StartArray() {
        IsReady();
        ExpectKey();
        init = true;
        if (nodes_stack_.back()->IsArray()) {
            auto *ptr = &std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back(Array{});
            nodes_stack_.emplace_back(ptr);
        } else {
            nodes_stack_.back()->GetValue() = Array{};
            auto *ptr = nodes_stack_.back();
            if (expect_value) {
                nodes_stack_.pop_back();
                expect_value = false;
            }
            nodes_stack_.emplace_back(ptr);
        }
        return static_cast<ArrayItemContext &>(*this);
    }

    Builder &Builder::EndArray() {
        IsReady();
        if (nodes_stack_.back()->IsArray()) {
            nodes_stack_.pop_back();
        } else {
            throw std::logic_error("Неверная команда");
        }
        return *this;
    }

    Node Builder::Build() {
        if (nodes_stack_.size() != 1 || !init) {
            throw std::logic_error("Неверная команда");
        }
        return root_;
    }

    void Builder::IsReady() {
        if (nodes_stack_.size() == 1 && init) {
            throw std::logic_error("Неверная команда");
        }
    }

    void Builder::ExpectKey() {
        if (nodes_stack_.back()->IsDict()) {
            throw std::logic_error("Неверная команда");
        }
    }

}