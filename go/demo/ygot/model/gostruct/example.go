package gostruct

//go:generate go run ../../../../../../../github.com/openconfig/ygot/generator/generator.go -include_model_data -path=yang -output_file=generated.go -package_name=gostruct -generate_fakeroot -fakeroot_name=device ../yang/example.yang