package main // import "github.com/neoul/libydb/go/ygot"

import (
	"fmt"
	"os"

	"github.com/neoul/libydb/go/ydb"
	"github.com/neoul/libydb/go/ygot/model/schema"
	model "github.com/neoul/libydb/go/ygot/model/schema"
	"github.com/sirupsen/logrus"

	"github.com/openconfig/goyang/pkg/yang"
	"github.com/openconfig/ygot/ytypes"
)

var (
	// Schema is schema information generated by ygot
	Schema *ytypes.Schema
	// Entries is yang.Entry list rearranged by name
	Entries map[string][]*yang.Entry
	ylog    *logrus.Entry
)

func init() {
	ylog = ydb.GetLogger("ydb2ygot")

	schema, err := model.Schema()
	if err != nil {
		ylog.Panicf("%s\n", err)
	}
	Schema = schema
	Entries = make(map[string][]*yang.Entry)
	for _, branch := range schema.SchemaTree {
		entries, _ := Entries[branch.Name]
		entries = append(entries, branch)
		for _, leaf := range branch.Dir {
			entries = append(entries, leaf)
		}
		Entries[branch.Name] = entries
		// if branch.Annotation["schemapath"] == "/" {
		// 	SchemaRoot = branch
		// }
	}
	for _, i := range Entries {
		for _, j := range i {
			ylog.Debug(j)
		}
	}
	ylog.Debug(model.ΓModelData)
}

func find(entry *yang.Entry, keys ...string) *yang.Entry {
	var found *yang.Entry
	if entry == nil {
		return nil
	}
	if len(keys) > 1 {
		found = entry.Dir[keys[0]]
		if found == nil {
			return nil
		}
		found = find(found, keys[1:]...)
	} else {
		found = entry.Dir[keys[0]]
	}
	return found
}

func main() {
	example := Schema.Root
	ylog.Debug(example)

	// db, close := ydb.Open("mydb")
	// defer close()
	// r, err := os.Open("model/data/example.yaml")
	// defer r.Close()
	// if err != nil {
	// 	ylog.Fatal(err)
	// }
	// dec := db.NewDecoder(r)
	// dec.Decode()

	// var user interface{}
	// user, err = db.Convert(ydb.RetrieveAll())
	// ylog.Debug(user)

	// var user map[string]interface{}
	// user = map[string]interface{}{}
	// _, err = db.Convert(ydb.RetrieveAll(), ydb.RetrieveStruct(user))
	// ylog.Debug(user)

	// gs := schema.Device{}
	// _, err = db.Convert(ydb.RetrieveAll(), ydb.RetrieveStruct(&gs))
	// ylog.Debug(gs)
	// fmt.Println(*gs.Country["United Kingdom"].Name, *gs.Country["United Kingdom"].CountryCode, *gs.Country["United Kingdom"].DialCode)
	// fmt.Println(*&gs.Company.Address, gs.Company.Enumval)

	// gs := schema.Device{}
	// _, err = db.Convert(ydb.RetrieveAll(), ydb.RetrieveStruct(&gs))
	// ylog.Debug(gs)
	// fmt.Println(*gs.Country["United Kingdom"].Name, *gs.Country["United Kingdom"].CountryCode, *gs.Country["United Kingdom"].DialCode)
	// fmt.Println(*&gs.Company.Address, gs.Company.Enumval)

	ydb.InitChildenOnSet = false
	gs := schema.Device{}
	db, close := ydb.OpenWithTargetStruct("running", &gs)
	defer close()
	r, err := os.Open("model/data/example-ydb.yaml")
	defer r.Close()
	if err != nil {
		ylog.Fatal(err)
	}
	dec := db.NewDecoder(r)
	dec.Decode()
	ydb.DebugValueString(gs, 5, func(x ...interface{}) { fmt.Print(x...) })
	fmt.Println("")
	fmt.Println(*gs.Country["United Kingdom"].Name, *gs.Country["United Kingdom"].CountryCode, *gs.Country["United Kingdom"].DialCode)
	fmt.Println(*&gs.Company.Address, gs.Company.Enumval)
	fmt.Println("")
}
