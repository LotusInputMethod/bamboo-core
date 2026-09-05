/*
 * Bamboo - A Vietnamese Input method editor
 * Copyright (C) Luong Thanh Lam <ltlam93@gmail.com>
 *
 * This software is licensed under the MIT license. For more information,
 * see <https://github.com/LotusInputMethod/bamboo-core/blob/master/LICENSE>.
 */

package bamboo

import "sort"

const UNICODE = "Unicode"

func Encode(charsetName string, input string) string {
	if charsetName == UNICODE {
		return input
	}
	var output string
	if charset, found := charsetDefinitions[charsetName]; found {
		for _, chr := range input {
			if out, found := charset[chr]; found {
				output = output + out
			} else {
				output = output + string(chr)
			}
		}
	} else {
		output = input
	}
	return output
}

func GetCharsetNames() []string {
	var others []string
	for cs := range charsetDefinitions {
		others = append(others, cs)
	}
	sort.Strings(others)

	names := make([]string, 0, len(others)+1)
	names = append(names, UNICODE)
	names = append(names, others...)
	return names
}
