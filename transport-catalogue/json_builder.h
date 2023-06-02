#pragma once

#include <sstream>
#include <stack>

#include "json.h"

namespace json {
    class ArrayItemContext;

    class KeyItemContext;

    class DictItemContext;

    class Builder {

    public:
        Builder();

        template<typename T = Builder>
        T &Value(Node::Value value);

        DictItemContext &StartDict();

        Builder &EndDict();

        ArrayItemContext &StartArray();

        Builder &EndArray();

        json::Node Build();

        KeyItemContext &Key(std::string key);

    private:
        Node root_;
        bool expect_value = false;
        bool init = false;
        std::vector<Node *> nodes_stack_{&root_};

        void IsReady();

        void ExpectKey();
    };

    template<typename T>
    T &Builder::Value(Node::Value value) {
        IsReady();
        ExpectKey();
        init = true;
        if (nodes_stack_.back()->IsArray()) {
            std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back(value);
        } else {
            nodes_stack_.back()->GetValue() = value;
        }
        if (expect_value) {
            nodes_stack_.pop_back();
            expect_value = false;
        }
        return static_cast<T &>(*this);;
    }

    class DictItemContext : public Builder {
    public:
        template<typename T>
        T Value(Node::Value value) = delete;

        Builder &Value(Node::Value value) = delete;

        DictItemContext &StartDict() = delete;

        ArrayItemContext &StartArray() = delete;

        Builder &EndArray() = delete;

        json::Node Build() = delete;
    };

    class ArrayItemContext : public Builder {
    public:
        Builder &EndDict() = delete;

        json::Node Build() = delete;

        KeyItemContext &Key(std::string key) = delete;

        ArrayItemContext &Value(Node::Value value) {
            return Builder::Value<ArrayItemContext>(std::move(value));
        }
    };

    class KeyItemContext : public Builder {
    public:
        Builder &EndDict() = delete;

        Builder &EndArray() = delete;

        json::Node Build() = delete;

        KeyItemContext &Key(std::string key) = delete;

        DictItemContext &Value(Node::Value value) {
            return Builder::Value<DictItemContext>(std::move(value));
        }
    };

}