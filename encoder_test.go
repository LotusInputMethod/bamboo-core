package bamboo

import (
	"reflect"
	"testing"
)

func TestGetCharsetNames(t *testing.T) {
	names1 := GetCharsetNames()
	names2 := GetCharsetNames()

	if len(names1) == 0 {
		t.Fatalf("GetCharsetNames returned empty slice")
	}

	if names1[0] != UNICODE {
		t.Errorf("First charset must be %s, got %s", UNICODE, names1[0])
	}

	if !reflect.DeepEqual(names1, names2) {
		t.Errorf("GetCharsetNames order is non-deterministic: %v vs %v", names1, names2)
	}

	expectedCount := len(charsetDefinitions) + 1
	if len(names1) != expectedCount {
		t.Errorf("Expected %d charset names, got %d", expectedCount, len(names1))
	}
}

func TestEncode(t *testing.T) {
	input := "Tiếng Việt"

	if got := Encode(UNICODE, input); got != input {
		t.Errorf("Encode Unicode failed. Got %s, expected %s", got, input)
	}

	if got := Encode("NonExistentCharset", input); got != input {
		t.Errorf("Encode non-existent charset failed. Got %s, expected %s", got, input)
	}

	for name := range charsetDefinitions {
		out := Encode(name, input)
		if len(out) == 0 {
			t.Errorf("Encode %s returned empty result for %s", name, input)
		}
	}
}
