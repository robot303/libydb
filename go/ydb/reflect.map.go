package ydb

import (
	"fmt"
	"reflect"
)

// map operations
// The settable m is the pointed by ptr value or structField.

// MapFind - Search an element by key.
func MapFind(m interface{}, key interface{}) (reflect.Value, bool) {
	v := reflect.ValueOf(m)
	return ValMapFind(v, key)
}

// map operations for reflect.Value

// ValMapFind - Search an element by the key.
func ValMapFind(v reflect.Value, key interface{}) (reflect.Value, bool) {
	if v.Kind() != reflect.Map {
		return reflect.Value{}, false
	}
	mt := v.Type()
	kt := mt.Key()
	if IsTypeInterface(kt) { // That means it is not a specified type.
		kt = reflect.TypeOf(key)
	}
	kv, err := ValScalarNew(kt, key)
	if err != nil || !kv.IsValid() {
		return reflect.Value{}, false
	}
	ev := v.MapIndex(kv)
	if !ev.IsValid() {
		return reflect.Value{}, false
	}
	return ev, true
}

// ValMapSet - Set an element to the map.
func ValMapSet(v reflect.Value, key interface{}, element interface{}) error {
	if v.Kind() != reflect.Map {
		return fmt.Errorf("not a map")
	}
	t := v.Type()
	kt := t.Key()
	if IsTypeInterface(kt) { // That means it is not a specified type.
		kt = reflect.TypeOf(key)
	}
	kv, err := ValScalarNew(kt, key)
	if err != nil || !kv.IsValid() {
		return fmt.Errorf("invalid key: %s", key)
	}
	var ev reflect.Value
	var et reflect.Type
	if IsTypeInterface(t.Elem()) { // That means it is not a specified type.
		if element == nil {
			return fmt.Errorf("not specified key or element type for %s", key)
		}
		et = reflect.TypeOf(element)
	} else {
		et = t.Elem()
	}
	if IsTypeScalar(et) {
		ev, err = ValScalarNew(et, element)
	} else {
		ev, err = ValNew(et)
	}
	if err != nil || !ev.IsValid() {
		return fmt.Errorf("invalid element: %s", element)
	}
	v.SetMapIndex(kv, ev)
	return nil
}

// ValMapUnset - Remove an element from the map
func ValMapUnset(v reflect.Value, key interface{}) error {
	if v.Kind() != reflect.Map {
		return fmt.Errorf("not a map")
	}
	mt := v.Type()
	kt := mt.Key()
	if IsTypeInterface(kt) { // That means it is not a specified type.
		kt = reflect.TypeOf(key)
	}
	kv, err := ValScalarNew(kt, key)
	if err != nil || !kv.IsValid() {
		return fmt.Errorf("invalid key: %s", key)
	}
	v.SetMapIndex(kv, reflect.Value{})
	return nil
}