import json
import math
import sys
# import time

with open('./1_word.txt', 'r', encoding='utf-8') as f:
    pinyin_chinese_mapping = json.load(f)
    
with open('./2_word.txt', 'r', encoding='utf-8') as g:
    bigram_counts = json.load(g)
    
def get_total_monogram(pinyin):
    if pinyin in monogram_dict:
        return monogram_dict[pinyin]

    total = 0
    for i in range(0, len(pinyin_chinese_mapping[pinyin]["words"])):
        total += pinyin_chinese_mapping[pinyin]["counts"][i]
    
    monogram_dict[pinyin] = total
    return total
    
def get_monogram_freq(pinyin, hanzi):
    expect = 0
    for i in range(0, len(pinyin_chinese_mapping[pinyin]["words"])):
        if pinyin_chinese_mapping[pinyin]["words"][i] == hanzi:
            expect = pinyin_chinese_mapping[pinyin]["counts"][i]
    return expect

def calc_prob(freq, total):
    # Applies for all circumstances
    if total == 0 or freq == 0:
        return 1000000000
    return -math.log(freq / total)
    
def get_conditional_freq(first_pinyin, second_pinyin, first_hanzi, second_hanzi):
    # global func_time
    # func_start_time = time.time()
    key_pinyin = first_pinyin + " " + second_pinyin 
    key_hanzi = first_hanzi + " " + second_hanzi
    
    # expect = 0
    if key_pinyin not in bigram_dict or key_hanzi not in bigram_dict[key_pinyin]:
        return 0
    return bigram_dict[key_pinyin][key_hanzi]

monogram_dict = {} # The value of "pinyin" is the total appearance of this pinyin
hanzi_log_prob_dict = {} # The value of "first_pinyin second_pinyin" is the total appearacne of this combination
bigram_dict = {}
viterbi_chart = []
# zero_time = 0
# non_zero_time = 0
# func_time = 0
        
def viterbi(sentence):
    # global zero_time
    # global non_zero_time
    global viterbi_chart 
    
    total_number = len(sentence)
    # First, build chart
    viterbi_chart = []
    for i in range(0, total_number): 
        pinyin_dict = {}
        for j in pinyin_chinese_mapping[sentence[i]]["words"]:
            pinyin_dict[j] = [0, ' ', 0]
        viterbi_chart.append(pinyin_dict)
    
    # Next, calculate probability
    for i in range(0, total_number):
        word_num = len(pinyin_chinese_mapping[sentence[i]]["words"])
        if i == 0:
            # zero_start_time = time.time()
            # First, check whether sentence[i] has become a key of monogram_dict.
            # If so, just get the data from there.
            # Else, calc the total frequency of this pinyin and store it in monogram_dict
            total = get_total_monogram(sentence[i])
            for j in range(0, word_num):
                freq = hanzi_log_prob_dict[sentence[i]][pinyin_chinese_mapping[sentence[i]]["words"][j]]
                log_prob = calc_prob(freq, total) # Better memorize this number. 
                letter = pinyin_chinese_mapping[sentence[i]]["words"][j]
                viterbi_chart[i][letter] = [log_prob, ' ', 0]
            # zero_end_time = time.time()
            # zero_time += zero_end_time - zero_start_time
        else:
            # non_zero_start_time = time.time()
            # First, calculate the total freq of each hanzi appeared in the previous viterbi chart
            for j in range(0, word_num): 
                # For all the hanzi searched by now
                cond_prob = float('inf')
                viterbi_chart[i][pinyin_chinese_mapping[sentence[i]]["words"][j]] = [1000000000, list(viterbi_chart[i-1].keys())[0], 0]
                flag = 0
                for index, k in enumerate(viterbi_chart[i - 1]):
                    if flag == 1 and viterbi_chart[i - 1][k][0] > 100000:
                        continue
                    # For all the hanzi searched one step ahead
                    cond_freq = get_conditional_freq(sentence[i-1], 
                    sentence[i], k, pinyin_chinese_mapping[sentence[i]]["words"][j])
                    # in case that new_prob is 0
                    calculated_prob = calc_prob(cond_freq, hanzi_log_prob_dict[sentence[i-1]][k])
                    new_val = calculated_prob + viterbi_chart[i - 1][k][0]
                    if new_val < 100000:
                        flag = 1
                    if new_val < cond_prob:
                        cond_prob = new_val
                        viterbi_chart[i][pinyin_chinese_mapping[sentence[i]]["words"][j]][0] = cond_prob
                        viterbi_chart[i][pinyin_chinese_mapping[sentence[i]]["words"][j]][1] = k
                        viterbi_chart[i][pinyin_chinese_mapping[sentence[i]]["words"][j]][2] = index
            # non_zero_end_time = time.time()
            # non_zero_time += non_zero_end_time - non_zero_start_time

    # Backtracking
    stack = []
    min_value = float('inf')
    min_key = None  
    
    for key, value in viterbi_chart[total_number-1].items():
        if value[0] < min_value:  
            min_value = value[0]  
            min_key = key 
    
    prev_key = viterbi_chart[total_number-1][min_key][1]
    stack.append(min_key)
    for i in range(total_number - 1, 0, -1):
        stack.append(prev_key)
        prev_key = viterbi_chart[i - 1][prev_key][1]
    
    ret = stack[::-1]
    return ret

def input_pinyin():
    while True: 
        try: 
            line = input()  
        except EOFError:
            break  
        sentence = line.strip() 
        sentence = sentence.split()
        result = viterbi(sentence)
        otp = ''.join(result)
        print(otp)
        
def input_from_file():
    with open('std_input.txt', 'r', encoding='utf-8') as file:
        lines = file.readlines()
        for line in lines:
            sentence = line.strip() 
            sentence = sentence.split()
            result = viterbi(sentence)
            otp = ''.join(result)
            print(otp)

def preproceess():
    for pinyin in pinyin_chinese_mapping:
        hanzi_log_prob_dict[pinyin] = {}
        for index, hanzi in enumerate(pinyin_chinese_mapping[pinyin]["words"]):
            hanzi_log_prob_dict[pinyin][hanzi] = pinyin_chinese_mapping[pinyin]["counts"][index]
    for pinyin in bigram_counts:
        bigram_dict[pinyin] = {}
        for index, word in enumerate(bigram_counts[pinyin]["words"]):
            bigram_dict[pinyin][word] = bigram_counts[pinyin]["counts"][index]
        

def main():
    # start_time = time.time()
    preproceess()
    input_pinyin()
    # with open('output.txt', 'w') as f:
    #     sys.stdout = f
    #     # start_time = time.time()
    #     input_from_file()
    #     sys.stdout = sys.__stdout__
main()