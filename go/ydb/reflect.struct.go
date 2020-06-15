package ydb

import (
	"fmt"
	"reflect"
)

// ValStructFieldFind - Find a struct field by the name (struct field name or Tag) from sv (struct)
func ValStructFieldFind(sv reflect.Value, name interface{}) (reflect.StructField, reflect.Value, bool) {
	if !sv.IsValid() {
		return reflect.StructField{}, reflect.Value{}, false
	}
	st := sv.Type()
	fnv, err := ValScalarNew(reflect.TypeOf(""), name)
	if !fnv.IsValid() || err != nil {
		return reflect.StructField{}, reflect.Value{}, false
	}
	sname := fnv.Interface().(string)
	ft, ok := st.FieldByName(sname)
	if ok {
		fv := sv.FieldByName(sname)
		return ft, fv, true
	}
	if EnableTagLookup {
		for i := 0; i < sv.NumField(); i++ {
			fv := sv.Field(i)
			ft := st.Field(i)
			if !fv.IsValid() || !fv.CanSet() {
				continue
			}
			if n, ok := ft.Tag.Lookup(TagLookupKey); ok && n == sname {
				return ft, fv, true
			}
		}
	}
	return reflect.StructField{}, reflect.Value{}, false
}

// ValStructFieldSet - Set the field of the struct.
func ValStructFieldSet(sv reflect.Value, name interface{}, val interface{}) error {
	if !sv.IsValid() {
		return fmt.Errorf("invalid struct")
	}
	ft, fv, ok := ValStructFieldFind(sv, name)
	if !ok {
		return fmt.Errorf("%s not found in %s", name, sv.Type())
	}
	if !fv.CanSet() {
		return fmt.Errorf("not settable field %s", name)
	}
	ftt := ft.Type
	if IsTypeInterface(ftt) { // That means it is not a specified type.
		if val == nil {
			return fmt.Errorf("not specified field or val type for %s", name)
		}
		ftt = reflect.TypeOf(val)
		if IsTypeScalar(ftt) {
			return ValScalarSet(fv, val)
		}
		nv := reflect.ValueOf(val)
		fv.Set(nv)
		return nil
	}
	if IsTypeScalar(ftt) {
		return ValScalarSet(fv, val)
	}
	nv, err := ValNew(ftt)
	if err != nil {
		return err
	}
	fv.Set(nv)
	return nil
}

// ValStructFieldUnset - Remove the field of the struct.
func ValStructFieldUnset(sv reflect.Value, name interface{}) error {
	if !sv.IsValid() {
		return fmt.Errorf("invalid struct")
	}
	ft, fv, ok := ValStructFieldFind(sv, name)
	if !ok {
		return fmt.Errorf("%s not found in %s", name, sv.Type())
	}
	if IsTypeScalar(ft.Type) {
		return ValScalarSet(fv, "")
	}
	nv, err := ValNew(ft.Type)
	if err != nil {
		return err
	}
	fv.Set(nv)
	return nil
}

// ValStructNew - Create new struct value and fill out all struct field.
func ValStructNew(t reflect.Type) (reflect.Value, error) {
	if t == reflect.TypeOf(nil) {
		return reflect.Value{}, fmt.Errorf("nil type")
	}
	if t.Kind() != reflect.Struct {
		return reflect.Value{}, fmt.Errorf("not slice")
	}
	pv := reflect.New(t)
	pve := pv.Elem()
	for i := 0; i < pve.NumField(); i++ {
		fv := pve.Field(i)
		ft := pve.Type().Field(i)
		if !fv.IsValid() || !fv.CanSet() {
			continue
		}
		switch ft.Type.Kind() {
		case reflect.Array, reflect.Complex64, reflect.Complex128, reflect.Chan:
			return reflect.Value{}, fmt.Errorf("not supported type: %s", t.Kind())
		case reflect.Map:
			fv.Set(reflect.MakeMap(ft.Type))
		case reflect.Slice:
			fv.Set(reflect.MakeSlice(ft.Type, 0, 0))
		case reflect.Struct:
			srv, err := ValStructNew(ft.Type)
			if err != nil {
				return srv, fmt.Errorf("%s not created", ft.Name)
			}
			fv.Set(srv)
		case reflect.Ptr:
			srv, err := ValNew(ft.Type)
			if err != nil {
				return srv, fmt.Errorf("%s not created (%v)", ft.Name, err)
			}
			fv.Set(srv)
		// case reflect.Chan:
		// 	fv.Set(reflect.MakeChan(ft.Type, 0))
		// case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
		// 	fv.SetInt(0)
		// case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64, reflect.Uintptr:
		// 	fv.SetUint(0)
		// case reflect.Float32, reflect.Float64:
		// 	fv.SetFloat(0)
		// case reflect.Bool:
		// 	fv.SetBool(false)
		// case reflect.String:
		// 	fv.SetString("")
		default:
		}
	}
	return pve, nil
}