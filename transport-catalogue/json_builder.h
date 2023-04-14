#pragma once

#include "json.h"
#include <optional>
#include <string>

namespace json {

    class Builder {
    private:
        std::optional<Node> root_;
        std::vector<Node *> nodes_stack_;
        std::optional<std::string> curr_key_;

        bool IsStarted();

        bool IsFinished();

        bool IsBuilt();

        bool built_ = false;


    public:
        class DictItemContext;

        class DictKeyContext;

        class ArrayItemContext;

        class DictItemContext {
        public:
            explicit DictItemContext(Builder &builder) : builder_(builder) {
            }

            DictKeyContext Key(std::string key);

            Builder &EndDict();

        private:
            Builder &builder_;
        };

        class DictKeyContext {
        public:
            explicit DictKeyContext(Builder &builder) : builder_(builder) {
            }

            DictItemContext Value(Node val);

            DictItemContext StartDict();

            ArrayItemContext StartArray();

        private:
            Builder &builder_;
        };

        class ArrayItemContext {
        public:
            explicit ArrayItemContext(Builder &builder) : builder_(builder) {
            }

            ArrayItemContext Value(Node val);

            ArrayItemContext StartArray();

            DictItemContext StartDict();

            Builder &EndArray();

        private:
            Builder &builder_;
        };

        Builder() = default;

        Builder &Value(Node val);

        DictKeyContext Key(std::string key);

        DictItemContext StartDict();

        Builder &EndDict();

        ArrayItemContext StartArray();

        Builder &EndArray();

        Node Build();

    private:
        [[nodiscard]] bool DoingDict() const;

        [[nodiscard]] bool DoingArray() const;

    };
}