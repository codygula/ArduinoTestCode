
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options
from pynput.keyboard import Key, Controller
import time
import json
# import keyboard

keyboard = Controller()

chrome_options = Options()
chrome_options.add_experimental_option("detach", True)



with open('config.json', 'r') as f:
  data = json.load(f)

# print(data['option1']['name'])


def openSite(optionNumber):
    print("openSite() called! ")
    site = data[f'option{optionNumber}']['url']
    name = data[f'option{optionNumber}']['name']
    print(name)
    driver = webdriver.Chrome(executable_path='/home/gula/Downloads/chromedriver-linux64/chromedriver', chrome_options=chrome_options)

    driver.get(site)
    time.sleep(1)
    keyboard.type("f")

def closeWindow():
    # This doesn't work
    print("closeWindow() Called! ")
    keyboard.type("<alt>+<F4>")



#openSite()


