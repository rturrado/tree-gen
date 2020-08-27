/** \file
 * Defines the base types used by \ref tree-gen to construct trees.
 */

#pragma once

#include <memory>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <sstream>
#include "tree-compat.hpp"
#include "tree-annotatable.hpp"

namespace tree {

/**
 * Namespace for the base types that \ref tree-gen relies on.
 */
namespace base {

// Forward declarations for classes.
template <class T>
class Maybe;
template <class T>
class One;
template <class T>
class Any;
template <class T>
class Many;
template <class T>
class OptLink;
template <class T>
class Link;

/**
 * Exception used by PointerMap to indicate not-well-formedness.
 */
class NotWellFormed : public std::runtime_error {
public:
    explicit NotWellFormed(const std::string &msg) : std::runtime_error(msg) {}
};

/**
 * Helper class used to assign unique, stable numbers the nodes in a tree, and
 * to check for well-formedness in terms of lack of duplicate nodes and dead
 * links.
 */
class PointerMap {
private:

    /**
     * Map of all raw pointers found so far with sequence numbers attached to
     * them.
     */
    std::unordered_map<const void*, size_t> map;

    /**
     * Internal implementation for add(), given only the raw pointer and the
     * name of its type for the error message.
     */
    size_t add_raw(const void *ptr, const char *name);

    /**
     * Internal implementation for get(), given only the raw pointer and the
     * name of its type for the error message.
     */
    size_t get_raw(const void *ptr, const char *name) const;

public:

    /**
     * Registers a node pointer and gives it a sequence number. If a duplicate
     * node is found, this raises a NotWellFormed.
     */
    template <class T>
    size_t add(const Maybe<T> &ob);

    /**
     * Returns the sequence number of a previously added node. If the node was
     * not previously added, this raises a NotWellFormed.
     */
    template <class T>
    size_t get(const Maybe<T> &ob) const;

    /**
     * Returns the sequence number of a previously added node. If the node was
     * not previously added, this raises a NotWellFormed.
     */
    template <class T>
    size_t get(const OptLink<T> &ob) const;

};

/**
 * Interface class for all tree nodes and the edge containers.
 */
class Completable {
public:
    virtual ~Completable() = default;

    /**
     * Traverses the tree to register all reachable Maybe/One nodes with the
     * given map. This also checks whether all One/Maybe nodes only appear once
     * in the tree (except through links). If there are duplicates, a
     * NotWellFormed exception is thrown.
     */
    virtual void find_reachable(PointerMap &map) const = 0;

    /**
     * Checks completeness of this node given a map of raw, internal Node
     * pointers to sequence numbers for all nodes reachable from the root. That
     * is:
     *  - all One, Link, and Many edges have (at least) one entry;
     *  - all the One entries internally stored by Any/Many have an entry;
     *  - all Link and filled OptLink nodes link to a node previously registered
     *    with the PointerMap.
     * If not complete, a NotWellFormed exception is thrown.
     */
    virtual void check_complete(const PointerMap &map) const = 0;

    /**
     * Checks whether the tree starting at this node is well-formed. That is:
     *  - all One, Link, and Many edges have (at least) one entry;
     *  - all the One entries internally stored by Any/Many have an entry;
     *  - all Link and filled OptLink nodes link to a node that's reachable from
     *    this node;
     *  - the nodes referred to be One/Maybe only appear once in the tree
     *    (except through links).
     * If it isn't well-formed, a NotWellFormed exception is thrown.
     */
    virtual void check_well_formed() const final;

    /**
     * Returns whether the tree starting at this node is well-formed. That is:
     *  - all One, Link, and Many edges have (at least) one entry;
     *  - all the One entries internally stored by Any/Many have an entry;
     *  - all Link and filled OptLink nodes link to a node that's reachable from
     *    this node;
     *  - the nodes referred to be One/Maybe only appear once in the tree
     *    (except through links).
     */
    virtual bool is_well_formed() const final;

};

/**
 * Base class for all tree nodes.
 */
class Base : public annotatable::Annotatable, public Completable {
};

/**
 * Convenience class for a reference to an optional tree node.
 */
template <class T>
class Maybe : public Completable {
protected:

    /**
     * The contained value.
     */
    std::shared_ptr<T> val;

public:

    /**
     * Constructor for an empty node.
     */
    Maybe() : val() {}

    /**
     * Constructor for an empty or filled node given an existing shared_ptr.
     */
    template <class S>
    explicit Maybe(const std::shared_ptr<S> &value) : val(std::static_pointer_cast<T>(value)) {}

    /**
     * Constructor for an empty or filled node given an existing shared_ptr.
     */
    template <class S>
    explicit Maybe(std::shared_ptr<S> &&value) : val(std::static_pointer_cast<T>(std::move(value))) {}

    /**
     * Constructor for an empty or filled node given an existing Maybe. Only
     * the reference is copied; use clone() if you want an actual copy.
     */
    template <class S>
    Maybe(const Maybe<S> &value) : val(std::static_pointer_cast<T>(value.val)) {}

    /**
     * Constructor for an empty or filled node given an existing Maybe. Only
     * the reference is copied; use clone() if you want an actual copy.
     */
    template <class S>
    Maybe(Maybe<S> &&value) : val(std::static_pointer_cast<T>(std::move(value.val))) {}

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    void set(const std::shared_ptr<S> &value) {
        val = std::static_pointer_cast<T>(value);
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    Maybe &operator=(const std::shared_ptr<S> &value) {
        set<S>(value);
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    void set(std::shared_ptr<S> &&value) {
        val = std::static_pointer_cast<T>(std::move(value));
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    Maybe &operator=(std::shared_ptr<S> &&value) {
        set<S>(std::move(value));
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    void set(const Maybe<S> &value) {
        val = std::static_pointer_cast<T>(value.get_ptr());
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    Maybe &operator=(const Maybe<S> &value) {
        set<S>(std::move(value));
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    void set(Maybe<S> &&value) {
        val = std::static_pointer_cast<T>(std::move(value.get_ptr()));
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    Maybe &operator=(Maybe<S> &&value) {
        set<S>(std::move(value));
    }

    /**
     * Sets the value to a NEW-ALLOCATED value pointed to AND TAKES OWNERSHIP.
     * In almost all cases, you should use set(make(...)) instead! This only
     * exists because the Yacc parser is one of the exceptions where you can't
     * help it, because the nodes have to be stored in a union while parsing,
     * and that can only be done with raw pointers.
     */
    template <class S>
    void set_raw(S *ob) {
        val = std::shared_ptr<T>(static_cast<T*>(ob));
    }

    /**
     * Removes the contained value.
     */
    void reset() {
        val.reset();
    }

    /**
     * Returns whether this Maybe is empty.
     */
    virtual bool empty() const {
        return val == nullptr;
    }

    /**
     * Returns whether this Maybe is empty.
     */
    size_t size() const {
        return val ? 1 : 0;
    }

    /**
     * Returns a mutable reference to the contained value. Raises an
     * `out_of_range` when the reference is empty.
     */
    T &deref() {
        if (!val) {
            throw std::out_of_range("dereferencing empty Maybe/One object");
        }
        return *val;
    }

    /**
     * Mutable dereference operator, shorthand for `deref()`.
     */
    T &operator*() {
        return deref();
    }

    /**
     * Mutable dereference operator, shorthand for `deref()`.
     */
    T *operator->() {
        return &deref();
    }

    /**
     * Returns a const reference to the contained value. Raises an
     * `out_of_range` when the reference is empty.
     */
    const T &deref() const {
        if (!val) {
            throw std::out_of_range("dereferencing empty Maybe/One object");
        }
        return *val;
    }

    /**
     * Constant dereference operator, shorthand for `deref()`.
     */
    const T &operator*() const {
        return deref();
    }

    /**
     * Constant dereference operator, shorthand for `deref()`.
     */
    const T *operator->() const {
        return &deref();
    }

    /**
     * Returns an immutable copy of the underlying shared_ptr.
     */
    const std::shared_ptr<T> &get_ptr() const {
        return val;
    }

    /**
     * Returns a mutable copy of the underlying shared_ptr.
     */
    std::shared_ptr<T> &get_ptr() {
        return val;
    }

    /**
     * Up- or downcasts this value. If the cast succeeds, the returned value
     * is nonempty and its shared_ptr points to the same data block as this
     * value does. If the cast fails, an empty Maybe is returned.
     */
    template <class S>
    Maybe<S> as() const {
        return Maybe<S>(std::dynamic_pointer_cast<S>(val));
    }

    /**
     * Makes a shallow copy of this value.
     */
    One<T> copy() const;

    /**
     * Makes a deep copy of this value.
     */
    One<T> clone() const;

    /**
     * Equality operator.
     */
    bool operator==(const Maybe& rhs) const {
        if (val && rhs.get_ptr()) {
            return *val == *rhs;
        } else {
            return val == rhs.get_ptr();
        }
    }

    /**
     * Inequality operator.
     */
    inline bool operator!=(const Maybe& rhs) const {
        return !(*this == rhs);
    }

    /**
     * Traverses the tree to register all reachable Maybe/One nodes with the
     * given map. This also checks whether all One/Maybe nodes only appear once
     * in the tree (except through links). If there are duplicates, a
     * NotWellFormed exception is thrown.
     */
    void find_reachable(PointerMap &map) const override {
        if (val) {
            map.add(*this);
            val->find_reachable(map);
        }
    }

    /**
     * Checks completeness of this node given a map of raw, internal Node
     * pointers to sequence numbers for all nodes reachable from the root. That
     * is:
     *  - all One, Link, and Many edges have (at least) one entry;
     *  - all the One entries internally stored by Any/Many have an entry;
     *  - all Link and filled OptLink nodes link to a node previously registered
     *    with the PointerMap.
     * If not complete, a NotWellFormed exception is thrown.
     */
    void check_complete(const PointerMap &map) const override {
        if (val) {
            val->check_complete(map);
        }
    }

    /**
     * Visit this object.
     */
    template <class V>
    void visit(V &visitor) {
        if (val) {
            val->visit(visitor);
        }
    }

};

/**
 * Convenience class for a reference to exactly one other tree node.
 */
template <class T>
class One : public Maybe<T> {
public:

    /**
     * Constructor for an empty (invalid) node.
     */
    One() : Maybe<T>() {}

    /**
     * Constructor for an empty or filled node given an existing shared_ptr.
     */
    template <class S>
    explicit One(const std::shared_ptr<S> &value) : Maybe<T>(value) {}

    /**
     * Constructor for an empty or filled node given an existing shared_ptr.
     */
    template <class S>
    explicit One(std::shared_ptr<S> &&value) : Maybe<T>(std::move(value)) {}

    /**
     * Constructor for an empty or filled node given an existing Maybe.
     */
    template <class S>
    One(const Maybe<S> &value) : Maybe<T>(value.get_ptr()) {}

    /**
     * Constructor for an empty or filled node given an existing Maybe.
     */
    template <class S>
    One(Maybe<S> &&value) : Maybe<T>(std::move(value.get_ptr())) {}

    /**
     * Checks completeness of this node given a map of raw, internal Node
     * pointers to sequence numbers for all nodes reachable from the root. That
     * is:
     *  - all One, Link, and Many edges have (at least) one entry;
     *  - all the One entries internally stored by Any/Many have an entry;
     *  - all Link and filled OptLink nodes link to a node previously registered
     *    with the PointerMap.
     * If not complete, a NotWellFormed exception is thrown.
     */
    void check_complete(const PointerMap &map) const override {
        if (!this->val) {
            std::ostringstream ss{};
            ss << "'One' edge of type " << typeid(T).name() << " is empty";
            throw NotWellFormed(ss.str());
        }
        this->val->check_complete(map);
    }

};

/**
 * Makes a shallow copy of this value.
 */
template <class T>
One<T> Maybe<T>::copy() const {
    if (val) {
        return val->copy();
    } else {
        return Maybe<T>();
    }
}

/**
 * Makes a deep copy of this value.
 */
template <class T>
One<T> Maybe<T>::clone() const {
    if (val) {
        return val->clone();
    } else {
        return Maybe<T>();
    }
}

/**
 * Constructs a One object, analogous to std::make_shared.
 */
template <class T, typename... Args>
One<T> make(Args... args) {
    return One<T>(std::make_shared<T>(args...));
}

/**
 * Convenience class for zero or more tree nodes.
 */
template <class T>
class Any : public Completable {
protected:

    /**
     * The contained vector.
     */
    std::vector<One<T>> vec;

public:

    /**
     * Adds the given value. No-op when the value is empty.
     */
    template <class S>
    void add(const Maybe<S> &ob, signed_size_t pos=-1) {
        if (ob.empty()) {
            return;
        }
        if (pos < 0 || (size_t)pos >= size()) {
            this->vec.emplace_back(
                std::static_pointer_cast<T>(ob.get_ptr()));
        } else {
            this->vec.emplace(this->vec.cbegin() + pos,
                              std::static_pointer_cast<T>(
                                  ob.get_ptr()));
        }
    }

    /**
     * Less versatile alternative for adding nodes with less verbosity.
     */
    template <class S, typename... Args>
    Any &emplace(Args... args) {
        this->vec.emplace_back(
            std::static_pointer_cast<T>(make<S>(args...).get_ptr()));
        return *this;
    }

    /**
     * Adds the NEW-ALLOCATED value pointed to AND TAKES OWNERSHIP. In almost
     * all cases, you should use add(make(...), pos) instead! This only exists
     * because the Yacc parser is one of the exceptions where you can't help
     * it, because the nodes have to be stored in a union while parsing, and
     * that can only be done with raw pointers.
     */
    template <class S>
    void add_raw(S *ob, signed_size_t pos=-1) {
        if (!ob) {
            throw std::runtime_error("add_raw called with nullptr!");
        }
        if (pos < 0 || (size_t)pos >= size()) {
            this->vec.emplace_back(std::shared_ptr<T>(static_cast<T*>(ob)));
        } else {
            this->vec.emplace(this->vec.cbegin() + pos, std::shared_ptr<T>(static_cast<T*>(ob)));
        }
    }

    /**
     * Extends this Any with another.
     */
    void extend(Any<T> &other) {
        this->vec.insert(this->vec.end(), other.vec.begin(), other.vec.end());
    }

    /**
     * Removes the object at the given index, or at the back if no index is
     * given.
     */
    void remove(signed_size_t pos=-1) {
        if (size() == 0) {
            return;
        }
        if (pos < 0 || pos >= size()) {
            pos = size() - 1;
        }
        this->vec.erase(this->vec.cbegin() + pos);
    }

    /**
     * Removes the contained values.
     */
    void reset() {
        vec.clear();
    }

    /**
     * Returns whether this Any is empty.
     */
    virtual bool empty() const {
        return vec.empty();
    }

    /**
     * Returns the number of elements in this Any.
     */
    size_t size() const {
        return vec.size();
    }

    /**
     * Returns a mutable reference to the contained value at the given index.
     * Raises an `out_of_range` when the reference is empty.
     */
    One<T> &at(size_t index) {
        return vec.at(index);
    }

    /**
     * Shorthand for `at()`. Unlike std::vector's operator[], this also checks
     * bounds.
     */
    One<T> &operator[] (size_t index) {
        return at(index);
    }

    /**
     * Returns an immutable reference to the contained value at the given
     * index. Raises an `out_of_range` when the reference is empty.
     */
    const One<T> &at(size_t index) const {
        return vec.at(index);
    }

    /**
     * Shorthand for `at()`. Unlike std::vector's operator[], this also checks
     * bounds.
     */
    const One<T> &operator[] (size_t index) const {
        return at(index);
    }

    /**
     * Returns a copy of the reference to the last value in the list. If the
     * list is empty, an empty reference is returned.
     */
    Maybe<T> back() const {
        if (vec.empty()) {
            return Maybe<T>();
        } else {
            return vec.back();
        }
    }

    /**
     * `begin()` for for-each loops.
     */
    typename std::vector<One<T>>::iterator begin() {
        return vec.begin();
    }

    /**
     * `begin()` for for-each loops.
     */
    typename std::vector<One<T>>::const_iterator begin() const {
        return vec.begin();
    }

    /**
     * `end()` for for-each loops.
     */
    typename std::vector<One<T>>::iterator end() {
        return vec.end();
    }

    /**
     * `end()` for for-each loops.
     */
    typename std::vector<One<T>>::const_iterator end() const {
        return vec.end();
    }

    /**
     * Makes a shallow copy of these values.
     */
    virtual Many<T> copy() const;

    /**
     * Makes a deep copy of these values.
     */
    virtual Many<T> clone() const;

    /**
     * Equality operator.
     */
    bool operator==(const Any& rhs) const {
        return vec == rhs.vec;
    }

    /**
     * Inequality operator.
     */
    inline bool operator!=(const Any& rhs) const {
        return !(*this == rhs);
    }

    /**
     * Traverses the tree to register all reachable Maybe/One nodes with the
     * given map. This also checks whether all One/Maybe nodes only appear once
     * in the tree (except through links). If there are duplicates, a
     * NotWellFormed exception is thrown.
     */
    void find_reachable(PointerMap &map) const override {
        for (auto &sptr : this->vec) {
            sptr.find_reachable(map);
        }
    }

    /**
     * Checks completeness of this node given a map of raw, internal Node
     * pointers to sequence numbers for all nodes reachable from the root. That
     * is:
     *  - all One, Link, and Many edges have (at least) one entry;
     *  - all the One entries internally stored by Any/Many have an entry;
     *  - all Link and filled OptLink nodes link to a node previously registered
     *    with the PointerMap.
     * If not complete, a NotWellFormed exception is thrown.
     */
    void check_complete(const PointerMap &map) const override {
        for (auto &sptr : this->vec) {
            sptr.check_complete(map);
        }
    }

    /**
     * Visit this object.
     */
    template <class V>
    void visit(V &visitor) {
        for (auto &sptr : this->vec) {
            if (!sptr.empty()) {
                sptr->visit(visitor);
            }
        }
    }

};

/**
 * Convenience class for one or more tree nodes.
 */
template <class T>
class Many : public Any<T> {
public:

    /**
     * Checks completeness of this node given a map of raw, internal Node
     * pointers to sequence numbers for all nodes reachable from the root. That
     * is:
     *  - all One, Link, and Many edges have (at least) one entry;
     *  - all the One entries internally stored by Any/Many have an entry;
     *  - all Link and filled OptLink nodes link to a node previously registered
     *    with the PointerMap.
     * If not complete, a NotWellFormed exception is thrown.
     */
    void check_complete(const PointerMap &map) const override {
        if (this->empty()) {
            std::ostringstream ss{};
            ss << "'Many' edge of type " << typeid(T).name() << " is empty";
            throw NotWellFormed(ss.str());
        }
        Any<T>::check_complete(map);
    }

};

/**
 * Convenience class for a reference to an optional tree node.
 */
template <class T>
class OptLink : public Completable {
protected:

    /**
     * The linked value.
     */
    std::weak_ptr<T> val;

public:

    /**
     * Constructor for an empty link.
     */
    OptLink() : val() {}

    /**
     * Constructor for an empty or filled node given the node to link to.
     */
    template <class S>
    OptLink(const Maybe<S> &value) : val(std::static_pointer_cast<T>(value.get_ptr())) {}

    /**
     * Constructor for an empty or filled node given the node to link to.
     */
    template <class S>
    OptLink(Maybe<S> &&value) : val(std::static_pointer_cast<T>(std::move(value.get_ptr()))) {}

    /**
     * Constructor for an empty or filled node given an existing link.
     */
    template <class S>
    OptLink(const OptLink<S> &value) : val(std::static_pointer_cast<T>(value.get_ptr())) {}

    /**
     * Constructor for an empty or filled node given an existing link.
     */
    template <class S>
    OptLink(OptLink<S> &&value) : val(std::static_pointer_cast<T>(std::move(value.get_ptr()))) {}

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    void set(const Maybe<S> &value) {
        val = std::static_pointer_cast<T>(value.get_ptr());
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    OptLink &operator=(const Maybe<S> &value) {
        set<S>(std::move(value));
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    void set(Maybe<S> &&value) {
        val = std::static_pointer_cast<T>(value.get_ptr());
    }

    /**
     * Sets the value to a reference to the given object, or clears it if null.
     */
    template <class S>
    OptLink &operator=(Maybe<S> &&value) {
        set<S>(std::move(value));
    }

    /**
     * Removes the contained value.
     */
    void reset() {
        val.reset();
    }

    /**
     * Returns whether this Maybe is empty.
     */
    virtual bool empty() const {
        return val.expired();
    }

    /**
     * Returns whether this Maybe is empty.
     */
    size_t size() const {
        return val.expired() ? 0 : 1;
    }

    /**
     * Returns a mutable reference to the contained value. Raises an
     * `out_of_range` when the reference is empty.
     */
    T &deref() {
        if (val.expired()) {
            throw std::out_of_range("dereferencing empty or expired (Opt)Link object");
        }
        return *(val.lock());
    }

    /**
     * Mutable dereference operator, shorthand for `deref()`.
     */
    T &operator*() {
        return deref();
    }

    /**
     * Mutable dereference operator, shorthand for `deref()`.
     */
    T *operator->() {
        return &deref();
    }

    /**
     * Returns a const reference to the contained value. Raises an
     * `out_of_range` when the reference is empty.
     */
    const T &deref() const {
        if (val.expired()) {
            throw std::out_of_range("dereferencing empty or expired (Opt)Link object");
        }
        return *(val.lock());
    }

    /**
     * Constant dereference operator, shorthand for `deref()`.
     */
    const T &operator*() const {
        return deref();
    }

    /**
     * Constant dereference operator, shorthand for `deref()`.
     */
    const T *operator->() const {
        return &deref();
    }

    /**
     * Returns an immutable copy of the underlying shared_ptr.
     */
    std::shared_ptr<const T> get_ptr() const {
        return val.lock();
    }

    /**
     * Returns a mutable copy of the underlying shared_ptr.
     */
    std::shared_ptr<T> get_ptr() {
        return val.lock();
    }

    /**
     * Up- or downcasts this value. If the cast succeeds, the returned value
     * is nonempty and its shared_ptr points to the same data block as this
     * value does. If the cast fails, an empty Maybe is returned.
     */
    template <class S>
    Maybe<S> as() const {
        return Maybe<S>(std::dynamic_pointer_cast<S>(val.lock()));
    }

    /**
     * Equality operator.
     */
    bool operator==(const OptLink& rhs) const {
        auto lhs_ptr = get_ptr();
        auto rhs_ptr = rhs.get_ptr();
        if (lhs_ptr && rhs_ptr) {
            return *lhs_ptr == *rhs_ptr;
        } else {
            return lhs_ptr == rhs_ptr;
        }
    }

    /**
     * Inequality operator.
     */
    inline bool operator!=(const OptLink& rhs) const {
        return !(*this == rhs);
    }

    /**
     * Returns whether this link links to the given node.
     */
    template <class S>
    bool links_to(const Maybe<S> target) {
        return get_ptr() == std::dynamic_pointer_cast<T>(target.get_ptr());
    }

    /**
     * Traverses the tree to register all reachable Maybe/One nodes with the
     * given map. This also checks whether all One/Maybe nodes only appear once
     * in the tree (except through links). If there are duplicates, a
     * NotWellFormed exception is thrown.
     */
    void find_reachable(PointerMap &map) const override {
    }

    /**
     * Checks completeness of this node given a map of raw, internal Node
     * pointers to sequence numbers for all nodes reachable from the root. That
     * is:
     *  - all One, Link, and Many edges have (at least) one entry;
     *  - all the One entries internally stored by Any/Many have an entry;
     *  - all Link and filled OptLink nodes link to a node previously registered
     *    with the PointerMap.
     * If not complete, a NotWellFormed exception is thrown.
     */
    void check_complete(const PointerMap &map) const override {
        if (!this->empty()) {
            map.get(*this);
        }
    }

    /**
     * Visit this object.
     */
    template <class V>
    void visit(V &visitor) {
        if (!val.expired()) {
            val.lock()->visit(visitor);
        }
    }

};

/**
 * Makes a shallow copy of these values.
 */
template <class T>
Many<T> Any<T>::copy() const {
    Many<T> c{};
    for (auto &sptr : this->vec) {
        c.add(sptr.copy());
    }
    return c;
}

/**
 * Makes a deep copy of these values.
 */
template <class T>
Many<T> Any<T>::clone() const {
    Many<T> c{};
    for (auto &sptr : this->vec) {
        c.add(sptr.clone());
    }
    return c;
}

/**
 * Convenience class for a reference to exactly one other tree node.
 */
template <class T>
class Link : public OptLink<T> {
public:

    /**
     * Constructor for an empty (invalid) node.
     */
    Link() : OptLink<T>() {}

    /**
     * Constructor for an empty or filled node given the node to link to.
     */
    template <class S>
    Link(const Maybe<S> &value) : OptLink<T>(value) {}

    /**
     * Constructor for an empty or filled node given the node to link to.
     */
    template <class S>
    Link(Maybe<S> &&value) : OptLink<T>(std::move(value)) {}

    /**
     * Constructor for an empty or filled node given an existing link.
     */
    template <class S>
    Link(const OptLink<S> &value) : OptLink<T>(value) {}

    /**
     * Constructor for an empty or filled node given an existing link.
     */
    template <class S>
    Link(OptLink<S> &&value) : OptLink<T>(std::move(value)) {}

    /**
     * Checks completeness of this node given a map of raw, internal Node
     * pointers to sequence numbers for all nodes reachable from the root. That
     * is:
     *  - all One, Link, and Many edges have (at least) one entry;
     *  - all the One entries internally stored by Any/Many have an entry;
     *  - all Link and filled OptLink nodes link to a node previously registered
     *    with the PointerMap.
     * If not complete, a NotWellFormed exception is thrown.
     */
    void check_complete(const PointerMap &map) const override {
        if (this->empty()) {
            std::ostringstream ss{};
            ss << "'Link' edge of type " << typeid(T).name() << " is empty";
            throw NotWellFormed(ss.str());
        }
        map.get(*this);
    }

};

/**
 * Registers a node pointer and gives it a sequence number. If a duplicate
 * node is found, this raises a NotWellFormed.
 */
template <class T>
size_t PointerMap::add(const Maybe<T> &ob) {
    return add_raw(reinterpret_cast<const void*>(ob.get_ptr().get()), typeid(T).name());
}

/**
 * Returns the sequence number of a previously added node. If the node was
 * not previously added, this raises a NotWellFormed.
 */
template <class T>
size_t PointerMap::get(const Maybe<T> &ob) const {
    return get_raw(reinterpret_cast<const void*>(ob.get_ptr().get()), typeid(T).name());
}

/**
 * Returns the sequence number of a previously added node. If the node was
 * not previously added, this raises a NotWellFormed.
 */
template <class T>
size_t PointerMap::get(const OptLink<T> &ob) const {
    return get_raw(reinterpret_cast<const void*>(ob.get_ptr().get()), typeid(T).name());
}

} // namespace base
} // namespace tree
