import sys


with open(sys.argv[1]) as words_file:
    word_list = [line.rstrip() for line in words_file]
    # Take just one in every X
    word_list = word_list[::10]

    with open('words.inl', 'a') as out_file:
        for w in word_list:
            out_file.write(f'WORD("{w}")\n')
