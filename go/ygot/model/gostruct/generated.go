/*
Package gostruct is a generated package which contains definitions
of structs which represent a YANG schema. The generated schema can be
compressed by a series of transformations (compression was false
in this case).

This package was generated by /home/neoul/go/src/github.com/openconfig/ygot/genutil/names.go
using the following YANG input files:
	- ../yang/example.yang
Imported modules were sourced from:
	- yang/...
*/
package gostruct

import (
	"encoding/json"
	"fmt"
	"reflect"

	"github.com/openconfig/ygot/ygot"
	"github.com/openconfig/goyang/pkg/yang"
	"github.com/openconfig/ygot/ytypes"
	gpb "github.com/openconfig/gnmi/proto/gnmi"
)

// Binary is a type that is used for fields that have a YANG type of
// binary. It is used such that binary fields can be distinguished from
// leaf-lists of uint8s (which are mapped to []uint8, equivalent to
// []byte in reflection).
type Binary []byte

// YANGEmpty is a type that is used for fields that have a YANG type of
// empty. It is used such that empty fields can be distinguished from boolean fields
// in the generated code.
type YANGEmpty bool

var (
	SchemaTree map[string]*yang.Entry
)

func init() {
	var err error
	if SchemaTree, err = UnzipSchema(); err != nil {
		panic("schema error: " +  err.Error())
	}
}

// Schema returns the details of the generated schema.
func Schema() (*ytypes.Schema, error) {
	uzp, err := UnzipSchema()
	if err != nil {
		return nil, fmt.Errorf("cannot unzip schema, %v", err)
	}

	return &ytypes.Schema{
		Root: &Device{},
		SchemaTree: uzp,
		Unmarshal: Unmarshal,
	}, nil
}

// UnzipSchema unzips the zipped schema and returns a map of yang.Entry nodes,
// keyed by the name of the struct that the yang.Entry describes the schema for.
func UnzipSchema() (map[string]*yang.Entry, error) {
	var schemaTree map[string]*yang.Entry
	var err error
	if schemaTree, err = ygot.GzipToSchema(ySchema); err != nil {
		return nil, fmt.Errorf("could not unzip the schema; %v", err)
	}
	return schemaTree, nil
}

// Unmarshal unmarshals data, which must be RFC7951 JSON format, into
// destStruct, which must be non-nil and the correct GoStruct type. It returns
// an error if the destStruct is not found in the schema or the data cannot be
// unmarshaled. The supplied options (opts) are used to control the behaviour
// of the unmarshal function - for example, determining whether errors are
// thrown for unknown fields in the input JSON.
func Unmarshal(data []byte, destStruct ygot.GoStruct, opts ...ytypes.UnmarshalOpt) error {
	tn := reflect.TypeOf(destStruct).Elem().Name()
	schema, ok := SchemaTree[tn]
	if !ok {
		return fmt.Errorf("could not find schema for type %s", tn )
	}
	var jsonTree interface{}
	if err := json.Unmarshal([]byte(data), &jsonTree); err != nil {
		return err
	}
	return ytypes.Unmarshal(schema, destStruct, jsonTree, opts...)
}
// ΓModelData contains the catalogue information corresponding to the modules for
// which Go code was generated.
var ΓModelData = []*gpb.ModelData{
    {
		Name: "network",
		Organization: "Actus Networks",
	},
}

// Device represents the /device YANG schema element.
type Device struct {
	Company	*Network_Company	`path:"company" module:"network"`
	Country	map[string]*Network_Country	`path:"country" module:"network"`
	Married	YANGEmpty	`path:"married" module:"network"`
	Multikeylist	map[Device_Multikeylist_Key]*Network_Multikeylist	`path:"multikeylist" module:"network"`
	Operator	map[uint32]*Network_Operator	`path:"operator" module:"network"`
	Person	*string	`path:"person" module:"network"`
}

// IsYANGGoStruct ensures that Device implements the yang.GoStruct
// interface. This allows functions that need to handle this struct to
// identify it as being generated by ygen.
func (*Device) IsYANGGoStruct() {}

// Device_Multikeylist_Key represents the key for list Multikeylist of element /device.
type Device_Multikeylist_Key struct {
	Str	string	`path:"str"`
	Integer	uint32	`path:"integer"`
}

// NewCountry creates a new entry in the Country list of the
// Device struct. The keys of the list are populated from the input
// arguments.
func (t *Device) NewCountry(Name string) (*Network_Country, error){

	// Initialise the list within the receiver struct if it has not already been
	// created.
	if t.Country == nil {
		t.Country = make(map[string]*Network_Country)
	}

	key := Name

	// Ensure that this key has not already been used in the
	// list. Keyed YANG lists do not allow duplicate keys to
	// be created.
	if _, ok := t.Country[key]; ok {
		return nil, fmt.Errorf("duplicate key %v for list Country", key)
	}

	t.Country[key] = &Network_Country{
		Name: &Name,
	}

	return t.Country[key], nil
}

// NewMultikeylist creates a new entry in the Multikeylist list of the
// Device struct. The keys of the list are populated from the input
// arguments.
func (t *Device) NewMultikeylist(Str string, Integer uint32) (*Network_Multikeylist, error){

	// Initialise the list within the receiver struct if it has not already been
	// created.
	if t.Multikeylist == nil {
		t.Multikeylist = make(map[Device_Multikeylist_Key]*Network_Multikeylist)
	}

	key := Device_Multikeylist_Key{
		Str: Str,
		Integer: Integer,
	}

	// Ensure that this key has not already been used in the
	// list. Keyed YANG lists do not allow duplicate keys to
	// be created.
	if _, ok := t.Multikeylist[key]; ok {
		return nil, fmt.Errorf("duplicate key %v for list Multikeylist", key)
	}

	t.Multikeylist[key] = &Network_Multikeylist{
		Str: &Str,
		Integer: &Integer,
	}

	return t.Multikeylist[key], nil
}

// NewOperator creates a new entry in the Operator list of the
// Device struct. The keys of the list are populated from the input
// arguments.
func (t *Device) NewOperator(Asn uint32) (*Network_Operator, error){

	// Initialise the list within the receiver struct if it has not already been
	// created.
	if t.Operator == nil {
		t.Operator = make(map[uint32]*Network_Operator)
	}

	key := Asn

	// Ensure that this key has not already been used in the
	// list. Keyed YANG lists do not allow duplicate keys to
	// be created.
	if _, ok := t.Operator[key]; ok {
		return nil, fmt.Errorf("duplicate key %v for list Operator", key)
	}

	t.Operator[key] = &Network_Operator{
		Asn: &Asn,
	}

	return t.Operator[key], nil
}

// Validate validates s against the YANG schema corresponding to its type.
func (t *Device) Validate(opts ...ygot.ValidationOption) error {
	if err := ytypes.Validate(SchemaTree["Device"], t, opts...); err != nil {
		return err
	}
	return nil
}

// ΛEnumTypeMap returns a map, keyed by YANG schema path, of the enumerated types
// that are included in the generated code.
func (t *Device) ΛEnumTypeMap() map[string][]reflect.Type { return ΛEnumTypes }


// Network_Company represents the /network/company YANG schema element.
type Network_Company struct {
	Address	[]string	`path:"address" module:"network"`
	Enumval	E_Network_Company_Enumval	`path:"enumval" module:"network"`
	Name	*string	`path:"name" module:"network"`
}

// IsYANGGoStruct ensures that Network_Company implements the yang.GoStruct
// interface. This allows functions that need to handle this struct to
// identify it as being generated by ygen.
func (*Network_Company) IsYANGGoStruct() {}

// Validate validates s against the YANG schema corresponding to its type.
func (t *Network_Company) Validate(opts ...ygot.ValidationOption) error {
	if err := ytypes.Validate(SchemaTree["Network_Company"], t, opts...); err != nil {
		return err
	}
	return nil
}

// ΛEnumTypeMap returns a map, keyed by YANG schema path, of the enumerated types
// that are included in the generated code.
func (t *Network_Company) ΛEnumTypeMap() map[string][]reflect.Type { return ΛEnumTypes }


// Network_Country represents the /network/country YANG schema element.
type Network_Country struct {
	CountryCode	*string	`path:"country-code" module:"network"`
	DialCode	*uint32	`path:"dial-code" module:"network"`
	Name	*string	`path:"name" module:"network"`
}

// IsYANGGoStruct ensures that Network_Country implements the yang.GoStruct
// interface. This allows functions that need to handle this struct to
// identify it as being generated by ygen.
func (*Network_Country) IsYANGGoStruct() {}

// ΛListKeyMap returns the keys of the Network_Country struct, which is a YANG list entry.
func (t *Network_Country) ΛListKeyMap() (map[string]interface{}, error) {
	if t.Name == nil {
		return nil, fmt.Errorf("nil value for key Name")
	}

	return map[string]interface{}{
		"name": *t.Name,
	}, nil
}

// Validate validates s against the YANG schema corresponding to its type.
func (t *Network_Country) Validate(opts ...ygot.ValidationOption) error {
	if err := ytypes.Validate(SchemaTree["Network_Country"], t, opts...); err != nil {
		return err
	}
	return nil
}

// ΛEnumTypeMap returns a map, keyed by YANG schema path, of the enumerated types
// that are included in the generated code.
func (t *Network_Country) ΛEnumTypeMap() map[string][]reflect.Type { return ΛEnumTypes }


// Network_Multikeylist represents the /network/multikeylist YANG schema element.
type Network_Multikeylist struct {
	Integer	*uint32	`path:"integer" module:"network"`
	Ok	*bool	`path:"ok" module:"network"`
	Str	*string	`path:"str" module:"network"`
}

// IsYANGGoStruct ensures that Network_Multikeylist implements the yang.GoStruct
// interface. This allows functions that need to handle this struct to
// identify it as being generated by ygen.
func (*Network_Multikeylist) IsYANGGoStruct() {}

// ΛListKeyMap returns the keys of the Network_Multikeylist struct, which is a YANG list entry.
func (t *Network_Multikeylist) ΛListKeyMap() (map[string]interface{}, error) {
	if t.Integer == nil {
		return nil, fmt.Errorf("nil value for key Integer")
	}

	if t.Str == nil {
		return nil, fmt.Errorf("nil value for key Str")
	}

	return map[string]interface{}{
		"integer": *t.Integer,
		"str": *t.Str,
	}, nil
}

// Validate validates s against the YANG schema corresponding to its type.
func (t *Network_Multikeylist) Validate(opts ...ygot.ValidationOption) error {
	if err := ytypes.Validate(SchemaTree["Network_Multikeylist"], t, opts...); err != nil {
		return err
	}
	return nil
}

// ΛEnumTypeMap returns a map, keyed by YANG schema path, of the enumerated types
// that are included in the generated code.
func (t *Network_Multikeylist) ΛEnumTypeMap() map[string][]reflect.Type { return ΛEnumTypes }


// Network_Operator represents the /network/operator YANG schema element.
type Network_Operator struct {
	Asn	*uint32	`path:"asn" module:"network"`
	Name	*string	`path:"name" module:"network"`
}

// IsYANGGoStruct ensures that Network_Operator implements the yang.GoStruct
// interface. This allows functions that need to handle this struct to
// identify it as being generated by ygen.
func (*Network_Operator) IsYANGGoStruct() {}

// ΛListKeyMap returns the keys of the Network_Operator struct, which is a YANG list entry.
func (t *Network_Operator) ΛListKeyMap() (map[string]interface{}, error) {
	if t.Asn == nil {
		return nil, fmt.Errorf("nil value for key Asn")
	}

	return map[string]interface{}{
		"asn": *t.Asn,
	}, nil
}

// Validate validates s against the YANG schema corresponding to its type.
func (t *Network_Operator) Validate(opts ...ygot.ValidationOption) error {
	if err := ytypes.Validate(SchemaTree["Network_Operator"], t, opts...); err != nil {
		return err
	}
	return nil
}

// ΛEnumTypeMap returns a map, keyed by YANG schema path, of the enumerated types
// that are included in the generated code.
func (t *Network_Operator) ΛEnumTypeMap() map[string][]reflect.Type { return ΛEnumTypes }


// E_Network_Company_Enumval is a derived int64 type which is used to represent
// the enumerated node Network_Company_Enumval. An additional value named
// Network_Company_Enumval_UNSET is added to the enumeration which is used as
// the nil value, indicating that the enumeration was not explicitly set by
// the program importing the generated structures.
type E_Network_Company_Enumval int64

// IsYANGGoEnum ensures that Network_Company_Enumval implements the yang.GoEnum
// interface. This ensures that Network_Company_Enumval can be identified as a
// mapped type for a YANG enumeration.
func (E_Network_Company_Enumval) IsYANGGoEnum() {}

// ΛMap returns the value lookup map associated with  Network_Company_Enumval.
func (E_Network_Company_Enumval) ΛMap() map[string]map[int64]ygot.EnumDefinition { return ΛEnum; }

// String returns a logging-friendly string for E_Network_Company_Enumval.
func (e E_Network_Company_Enumval) String() string {
	return ygot.EnumLogString(e, int64(e), "E_Network_Company_Enumval")
}

const (
	// Network_Company_Enumval_UNSET corresponds to the value UNSET of Network_Company_Enumval
	Network_Company_Enumval_UNSET E_Network_Company_Enumval = 0
	// Network_Company_Enumval_enum1 corresponds to the value enum1 of Network_Company_Enumval
	Network_Company_Enumval_enum1 E_Network_Company_Enumval = 1
	// Network_Company_Enumval_enum2 corresponds to the value enum2 of Network_Company_Enumval
	Network_Company_Enumval_enum2 E_Network_Company_Enumval = 2
	// Network_Company_Enumval_enum3 corresponds to the value enum3 of Network_Company_Enumval
	Network_Company_Enumval_enum3 E_Network_Company_Enumval = 31
)


// ΛEnum is a map, keyed by the name of the type defined for each enum in the
// generated Go code, which provides a mapping between the constant int64 value
// of each value of the enumeration, and the string that is used to represent it
// in the YANG schema. The map is named ΛEnum in order to avoid clash with any
// valid YANG identifier.
var ΛEnum = map[string]map[int64]ygot.EnumDefinition{
	"E_Network_Company_Enumval": {
		1: {Name: "enum1"},
		2: {Name: "enum2"},
		31: {Name: "enum3"},
	},
}


var (
	// ySchema is a byte slice contain a gzip compressed representation of the
	// YANG schema from which the Go code was generated. When uncompressed the
	// contents of the byte slice is a JSON document containing an object, keyed
	// on the name of the generated struct, and containing the JSON marshalled
	// contents of a goyang yang.Entry struct, which defines the schema for the
	// fields within the struct.
	ySchema = []byte{
		0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xec, 0x5a, 0x5d, 0x4f, 0xdb, 0x3c,
		0x14, 0xbe, 0xef, 0xaf, 0x88, 0x7c, 0xdd, 0x57, 0x94, 0xc2, 0x3b, 0xd6, 0xde, 0x31, 0x3e, 0x34,
		0x89, 0x01, 0x13, 0x9b, 0x76, 0x33, 0x4d, 0x93, 0x97, 0x1c, 0x82, 0xd5, 0xc4, 0x8e, 0x6c, 0x07,
		0x88, 0xa6, 0xfe, 0xf7, 0x29, 0x75, 0x12, 0x92, 0xc6, 0x76, 0xdc, 0x15, 0x34, 0x3e, 0x7c, 0x89,
		0xfd, 0x38, 0xe7, 0xf8, 0x9c, 0xe7, 0x1c, 0x3f, 0x36, 0xfd, 0x3d, 0x0a, 0x82, 0x20, 0x40, 0x17,
		0x38, 0x05, 0x34, 0x0f, 0x50, 0x04, 0xb7, 0x24, 0x04, 0x34, 0x56, 0xa3, 0x67, 0x84, 0x46, 0x68,
		0x1e, 0xec, 0x56, 0x7f, 0x1e, 0x31, 0x7a, 0x4d, 0x62, 0x34, 0x0f, 0x26, 0xd5, 0xc0, 0x31, 0xe1,
		0x68, 0x1e, 0xa8, 0x4f, 0xac, 0x06, 0x42, 0x96, 0x66, 0x98, 0x16, 0x9d, 0xc1, 0xce, 0xf7, 0x6b,
		0xc0, 0xb8, 0x3b, 0xdd, 0x35, 0xd4, 0x0c, 0xaf, 0x1b, 0x6c, 0x26, 0x3e, 0x73, 0xb8, 0x26, 0xf7,
		0x3d, 0x33, 0x1d, 0x53, 0x14, 0xe4, 0x9a, 0x99, 0xd5, 0xf4, 0x17, 0x96, 0xf3, 0x10, 0xb4, 0x4b,
		0x95, 0x2b, 0x50, 0xdc, 0x31, 0x5e, 0x7a, 0x83, 0x32, 0x65, 0x65, 0xac, 0x07, 0x7e, 0xc4, 0xe2,
		0x90, 0xc7, 0x79, 0x0a, 0x54, 0xa2, 0x79, 0x20, 0x79, 0x0e, 0x06, 0x60, 0x0b, 0xb5, 0x72, 0xaa,
		0x87, 0x5a, 0x76, 0x46, 0x96, 0x6b, 0x7b, 0x5d, 0x0f, 0x72, 0x33, 0x81, 0xa3, 0x88, 0x83, 0x10,
		0xe6, 0xad, 0xd4, 0x91, 0xa8, 0x81, 0x06, 0xff, 0xaa, 0xe0, 0x4f, 0x0c, 0xd3, 0xa6, 0x24, 0xb8,
		0x24, 0xc3, 0x31, 0x29, 0xae, 0xc9, 0xd9, 0x38, 0x49, 0x1b, 0x27, 0xcb, 0x3d, 0x69, 0xfa, 0xe4,
		0x19, 0x92, 0xd8, 0x7c, 0xf6, 0x6b, 0x91, 0x81, 0x5b, 0xa4, 0x84, 0xe4, 0x84, 0xc6, 0xb6, 0x60,
		0xd5, 0x45, 0xf3, 0x7e, 0x23, 0x0f, 0x3e, 0x11, 0x21, 0x0f, 0xa5, 0xe4, 0x76, 0x2f, 0xce, 0x09,
		0x3d, 0x49, 0xa0, 0x0c, 0x40, 0xc9, 0x2f, 0x9a, 0x27, 0x89, 0xc5, 0x91, 0x73, 0x7c, 0xef, 0x0e,
		0xbe, 0xe4, 0x11, 0x70, 0x88, 0x3e, 0x14, 0x15, 0x74, 0xe4, 0x16, 0x54, 0xcd, 0x76, 0x10, 0xd0,
		0x3c, 0xbd, 0xc5, 0xc9, 0x70, 0x01, 0xd4, 0x40, 0x5f, 0x00, 0x2f, 0xa8, 0x00, 0xca, 0xa4, 0x01,
		0xc7, 0x92, 0x30, 0xea, 0x52, 0x05, 0xfb, 0x16, 0xcc, 0x09, 0xcd, 0xd3, 0xd2, 0xe8, 0x72, 0x0b,
		0xb2, 0x51, 0xe5, 0xd6, 0x00, 0xd3, 0x56, 0x28, 0x4f, 0xb3, 0xb7, 0xd4, 0x67, 0x37, 0x3c, 0xce,
		0x0f, 0x29, 0x65, 0x52, 0xb1, 0x5a, 0x7b, 0xaa, 0x8b, 0xf0, 0x06, 0x52, 0x9c, 0x61, 0x79, 0x53,
		0x7a, 0xb7, 0x43, 0x41, 0xde, 0x31, 0xbe, 0xd8, 0xd1, 0x0b, 0x27, 0xb5, 0x42, 0xf2, 0x3c, 0x94,
		0x15, 0x3f, 0xd1, 0x85, 0x5a, 0xf0, 0xf3, 0xa8, 0x5a, 0x30, 0xd2, 0xbb, 0xd6, 0x72, 0x0b, 0x85,
		0x2c, 0xa7, 0x92, 0x5b, 0x55, 0x9b, 0x02, 0x78, 0xd5, 0xb6, 0xbd, 0x6a, 0xab, 0x62, 0xf9, 0x5f,
		0xc8, 0x22, 0x87, 0x7e, 0xd2, 0x41, 0xfb, 0xbe, 0xf2, 0xb6, 0xfb, 0x8a, 0xa6, 0x2e, 0x22, 0x82,
		0x13, 0x47, 0x2a, 0x3d, 0x40, 0x3d, 0x8f, 0x5e, 0x10, 0x8f, 0x72, 0x42, 0xe5, 0xde, 0xd4, 0x81,
		0x47, 0x07, 0x16, 0xc8, 0x15, 0xa6, 0x71, 0xf9, 0xb5, 0xef, 0xd6, 0xcd, 0xda, 0x83, 0x5d, 0xdf,
		0x0b, 0x06, 0xb3, 0xe2, 0x48, 0xa8, 0x1e, 0xfc, 0x1b, 0x4e, 0x72, 0xe8, 0x9f, 0x27, 0x46, 0xfc,
		0x29, 0xc7, 0x61, 0x79, 0x8e, 0x1e, 0x93, 0x98, 0xac, 0x6e, 0x1e, 0x93, 0xc1, 0x75, 0xcb, 0xb1,
		0xc3, 0x16, 0xf1, 0xfd, 0x93, 0x6f, 0x71, 0x7f, 0x36, 0x7d, 0xc2, 0x4d, 0x8e, 0xfe, 0x6e, 0xf6,
		0x87, 0x57, 0xc7, 0xbe, 0xfb, 0xfc, 0x63, 0x75, 0x7c, 0x06, 0x85, 0x81, 0x28, 0xf6, 0x87, 0x0b,
		0xa7, 0x07, 0x0b, 0xa7, 0x87, 0x0a, 0xfb, 0x03, 0xc5, 0xa3, 0xa9, 0x79, 0x9d, 0xa0, 0xb6, 0xaa,
		0x79, 0xb5, 0xc0, 0x41, 0xcd, 0xa7, 0x98, 0x73, 0x02, 0x91, 0x59, 0xcd, 0xd7, 0x00, 0xbd, 0x9a,
		0x9f, 0x78, 0x35, 0xdf, 0x4f, 0xb3, 0xb1, 0x60, 0x1e, 0x5e, 0x2b, 0xd2, 0x4c, 0x6a, 0xf3, 0x59,
		0xd7, 0xc7, 0x9e, 0x4b, 0xea, 0xf2, 0x44, 0x92, 0x05, 0x14, 0x09, 0x11, 0xd2, 0x92, 0xbf, 0x36,
		0xca, 0x5f, 0xc9, 0xb6, 0xbf, 0x92, 0x11, 0x2a, 0x21, 0x06, 0x3e, 0x7c, 0x7e, 0xd5, 0x40, 0x7f,
		0x84, 0x79, 0x01, 0xfd, 0x8c, 0x05, 0xf4, 0xe4, 0x0d, 0x08, 0xe8, 0xe9, 0x6c, 0x7f, 0xf6, 0xee,
		0x60, 0x3a, 0xfb, 0xff, 0x75, 0xe9, 0x68, 0xb6, 0x18, 0xee, 0x42, 0x6c, 0xe1, 0x1b, 0xd0, 0x4b,
		0x6a, 0x40, 0xbf, 0x18, 0x4b, 0x00, 0x3b, 0xfd, 0x13, 0x63, 0x77, 0x0b, 0xea, 0x08, 0xe9, 0x70,
		0x82, 0x95, 0x20, 0x4f, 0x1e, 0x7f, 0x01, 0x1b, 0xba, 0x80, 0x09, 0xc9, 0x03, 0xbd, 0xda, 0x79,
		0x55, 0xf7, 0x30, 0x8b, 0x94, 0xb6, 0x5c, 0xc6, 0xce, 0xdb, 0xab, 0x1c, 0x64, 0x3d, 0xcb, 0x80,
		0x63, 0xc9, 0xb8, 0x59, 0xd2, 0x37, 0x08, 0x2f, 0xe7, 0xb7, 0x97, 0xf3, 0x58, 0x50, 0x87, 0xdf,
		0xc4, 0x08, 0xea, 0x1b, 0xa1, 0x97, 0xf1, 0x5e, 0xc6, 0x7b, 0x19, 0xef, 0x9f, 0xc3, 0x7d, 0x13,
		0x7a, 0xde, 0x6a, 0xac, 0x7f, 0x58, 0xbd, 0x2a, 0x15, 0x66, 0x50, 0x3f, 0x16, 0x05, 0x76, 0x59,
		0xaf, 0x70, 0x50, 0x5f, 0x19, 0x70, 0xa1, 0x71, 0xa8, 0xc9, 0x69, 0x35, 0xef, 0x5f, 0xc3, 0x1f,
		0xf1, 0x35, 0xdc, 0x58, 0x27, 0x86, 0xfa, 0x68, 0x65, 0x6e, 0xd4, 0xb2, 0x68, 0x22, 0x14, 0x22,
		0xe2, 0x14, 0x2f, 0xe0, 0x8a, 0xb1, 0xfe, 0x46, 0xd7, 0x49, 0x86, 0xda, 0x53, 0x1d, 0x32, 0x1d,
		0xab, 0xdf, 0xba, 0x2b, 0x83, 0xa3, 0xe5, 0x1f, 0x00, 0x00, 0x00, 0xff, 0xff, 0x01, 0x00, 0x00,
		0xff, 0xff, 0x4d, 0x74, 0x1d, 0x81, 0x0a, 0x2f, 0x00, 0x00,
	}
)


// ΛEnumTypes is a map, keyed by a YANG schema path, of the enumerated types that
// correspond with the leaf. The type is represented as a reflect.Type. The naming
// of the map ensures that there are no clashes with valid YANG identifiers.
var ΛEnumTypes = map[string][]reflect.Type{
	"/company/enumval": []reflect.Type{
		reflect.TypeOf((E_Network_Company_Enumval)(0)),
	},
}
