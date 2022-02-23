Run on python 3, ubuntu 20

import sys
import os
import re
import pickle
from tqdm import tqdm                                                                     # for progress bar
import codecs
from collections import Counter

import nltk
nltk.download('stopwords')
nltk.download('punkt')
nltk.download('wordnet')
from nltk.corpus import stopwords
from nltk.tokenize import word_tokenize
from nltk.stem import WordNetLemmatizer




