/*
 * Bamboo - A Vietnamese Input method editor
 * Copyright (C) Luong Thanh Lam <ltlam93@gmail.com>
 * Copyright (C) Nguyễn Hoàng Kỳ  <nhktmdzhg@gmail.com>
 *
 * This software is licensed under the MIT license. For more information,
 * see <https://github.com/LotusInputMethod/bamboo-core/blob/master/LICENSE>.
 */

package bamboo

var firstConsonantSeqs = [][]string{
	{"b", "c", "ch", "d", "đ", "g", "gh", "gi", "h", "k", "kh", "kr", "l", "m",
		"n", "ng", "ngh", "nh", "p", "ph", "qu", "r", "s", "t", "th", "tr", "v", "x", "z"},
}

// Row index maps 1:1 to vcMatrix: vowel row i pairs with vcMatrix[i].
var vowelSeqs = [][]string{
	{"ê", "i", "ua", "uê", "uy", "y"},
	{"a", "iê", "oa", "uyê", "yê"},
	{"â", "ă", "e", "o", "oo", "ô", "ơ", "oe", "u", "ư", "uâ", "uô", "ươ"},
	{"oă"},
	{"uơ"},
	{"ai", "ao", "au", "âu", "ay", "ây", "eo", "êu", "ia", "iêu", "iu", "oai", "oao", "oay", "oeo", "oi", "ôi", "ơi", "ưa", "uây", "ui", "ưi", "uôi", "ươi", "ươu", "ưu", "uya", "uyu", "uêu", "yêu"},
	{"ă", "u"},
	{"i"},
}

var lastConsonantSeqs = [][]string{
	{"ch", "nh"},
	{"c", "ng"},
	{"m", "n", "p", "t"},
	{"k"},
	{"c"},
}

var vcMatrix = [][]int{
	{0, 2},
	{0, 1, 2},
	{1, 2},
	{1, 2},
	{},
	{},
	{3},
	{4},
}

func lookup(seq [][]string, input string, inputIsFull, inputIsComplete bool) []int {
	var ret []int
	var inputRunes = []rune(input)
	var inputLen = len(inputRunes)
	for index, row := range seq {
		for _, token := range row {
			var canvas = []rune(token)
			if len(canvas) < inputLen || (inputIsFull && len(canvas) > inputLen) {
				continue
			}
			var isMatch = true
			for k, ic := range inputRunes {
				if ic != canvas[k] && !(!inputIsComplete && AddMarkToTonelessChar(canvas[k], 0) == ic) {
					isMatch = false
					break
				}
			}
			if isMatch {
				ret = append(ret, index)
				break
			}
		}
	}
	return ret
}

func isValidCVC(fc, vo, lc string, inputIsFullComplete bool) bool {
	var ret bool
	var fcIndexes, voIndexes, lcIndexes []int
	// log.Printf("fc=%s vo=%s lc=%s ret=%v", fc, vo, lc, ret)
	if fc != "" {
		if fcIndexes = lookup(firstConsonantSeqs, fc, inputIsFullComplete || vo != "", true); fcIndexes == nil {
			return false
		}
	}
	if vo != "" {
		if voIndexes = lookup(vowelSeqs, vo, inputIsFullComplete || lc != "", inputIsFullComplete); voIndexes == nil {
			return false
		}
	}
	if lc != "" {
		if lcIndexes = lookup(lastConsonantSeqs, lc, inputIsFullComplete, true); lcIndexes == nil {
			return false
		}
	}
	if voIndexes == nil {
		// first consonant only
		return fcIndexes != nil
	}
	if lcIndexes != nil {
		// vowel + last consonant
		ret = isValidVC(voIndexes, lcIndexes)
	} else {
		// vowel only
		ret = true
	}
	return ret
}

func isValidVC(voIndexes, lcIndexes []int) bool {
	for _, vo := range voIndexes {
		for _, c := range vcMatrix[vo] {
			for _, lc := range lcIndexes {
				if c == lc {
					return true
				}
			}
		}
	}
	return false
}
