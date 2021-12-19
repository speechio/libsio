#include "gtest/gtest.h"

#include <string>
#include <fstream>
#include <sstream>

#include "sio/json.h"

// test are copied from https://github.com/nbsdx/SimpleJSON/tree/master/examples

using json::JSON;
using namespace std;

TEST(JSON, Prim) {
  JSON obj;

  obj = true;
  cout << "Value: " << boolalpha << obj.ToBool() << endl;

  obj = "Test String";
  cout << "Value: " << obj.ToString() << endl;

  obj = 2.2;
  cout << "Value: " << obj.ToFloat() << endl;

  obj = 3;
  cout << "Value: " << obj.ToInt() << endl;
}

TEST(JSON, LoadFromString) {
  JSON Int    = JSON::Load( " 123 " );
  JSON Float  = JSON::Load( " 123.234 " );
  JSON Str    = JSON::Load( "\"String\"" );
  JSON EscStr = JSON::Load( "\" \\\"Some\\/thing\\\" \"" );
  JSON Arr    = JSON::Load( "[1,2, true, false,\"STRING\", 1.5]" );
  JSON Obj    = JSON::Load( "{ \"Key\" : \"StringValue\","
                            "   \"Key2\" : true, "
                            "   \"Key3\" : 1234, "
                            "   \"Key4\" : null }" );
  
  cout << Int << endl;
  cout << Float << endl;
  cout << Str << endl;
  cout << EscStr << endl;
  cout << Arr << endl;
  cout << Obj << endl;

}

TEST(JSON, LoadFromFile) {
  JSON json_obj = json::Load("testdata/stt.json");  // SIO extended function
  //cout << j << endl;
  EXPECT_EQ(json_obj["model"].ToString(), "model_dir/model.bin");
  EXPECT_EQ(json_obj["sample_rate"].ToFloat(), 16000.0);
  EXPECT_EQ(json_obj["online"].ToBool(), true);
}

TEST(JSON, DumpToString) {
  JSON obj = json::Load("testdata/stt.json");

  std::string dump_str = obj.dump();
  cout << "dump_str: " << dump_str << endl;
}

TEST(JSON, JSON) {
  // Example of creating each type
  // You can also do JSON::Make( JSON::Class )
  JSON null;
  JSON Bool( true );
  JSON Str( "RawString" );
  JSON Str2( string( "C++String" ) );
  JSON Int( 1 );
  JSON Float( 1.2 );
  JSON Arr = json::Array();
  JSON Obj = json::Object();

  // Types can be overwritten by assigning
  // to the object again.
  Bool = false;
  Bool = "rtew";
  Bool = 1;
  Bool = 1.1;
  Bool = string( "asd" );

  // Append to Arrays, appending to a non-array
  // will turn the object into an array with the
  // first element being the value that's being
  // appended.
  Arr.append( 1 );
  Arr.append( "test" );
  Arr.append( false );

  // Access Array elements with operator[]( unsigned ).
  // Note that this does not do bounds checking, and 
  // returns a reference to a JSON object.
  JSON& val = Arr[0];

  // Arrays can be intialized with any elements and
  // they are turned into JSON objects. Variadic 
  // Templates are pretty cool.
  JSON Arr2 = json::Array( 2, "Test", true );

  // Objects are accessed using operator[]( string ).
  // Will create new pairs on the fly, just as std::map
  // would.
  Obj["Key1"] = 1.0;
  Obj["Key2"] = "Value";

  JSON Obj2 = json::Object();
  Obj2["Key3"] = 1;
  Obj2["Key4"] = Arr;
  Obj2["Key5"] = Arr2;
  
  // Nested Object
  Obj["Key6"] = Obj2;

  // Dump Obj to a string.
  cout << Obj << endl;

  // We can also use a more JSON-like syntax to create 
  // JSON Objects.
  JSON Obj3 = {
      "Key1", "Value",
      "Key2", true,
      "Key3", {
          "Key4", json::Array( "This", "Is", "An", "Array" ),
          "Key5", {
              "BooleanValue", true
          }
      }
  };

  cout << Obj3 << endl;
}


TEST(JSON, Array) {
  //using json = nlohmann::json;

  JSON array;

  array[2] = "Test2";
  cout << array << endl;
  array[1] = "Test1";
  cout << array << endl;
  array[0] = "Test0";
  cout << array << endl;
  array[3] = "Test4";
  cout << array << endl;

  // Arrays can be nested:
  JSON Array2;

  Array2[2][0][1] = true;

  cout << Array2 << endl;

}

TEST(JSON, Init) {
  JSON obj( {
      "Key", 1,
      "Key3", true,
      "Key4", nullptr,
      "Key2", {
          "Key4", "VALUE",
          "Arr", json::Array( 1, "Str", false )
      }
  } );

  cout << obj << endl;
}


void dumpArrayConst( const JSON &array ) {
    for( auto &j : array.ArrayRange() )
        std::cout << "Value: " << j << "\n";
}

void dumpArray( JSON &array ) {
    for( auto &j : array.ArrayRange() )
        std::cout << "Value: " << j << "\n";
}

void dumpObjectConst( const JSON &object ) {
    for( auto &j : object.ObjectRange() )
        std::cout << "Object[ " << j.first << " ] = " << j.second << "\n";
}

void dumpObject( JSON &object ) {
    for( auto &j : object.ObjectRange() )
        std::cout << "Object[ " << j.first << " ] = " << j.second << "\n";
}

TEST(JSON, Iter) {
  JSON array = JSON::Make( JSON::Class::Array );
  JSON obj   = JSON::Make( JSON::Class::Object );

  array[0] = "Test0";
  array[1] = "Test1";
  array[2] = "Test2";
  array[3] = "Test3";

  obj[ "Key0" ] = "Value1";
  obj[ "Key1" ] = array;
  obj[ "Key2" ] = 123;

  dumpArray( array );
  dumpObject( obj );

  dumpArrayConst( array );
  dumpObjectConst( obj );
}
