import win32api, win32gui, win32ui, win32con

import cv2
import numpy
import json
import sys
import ctypes
import winreg
import ast
import zipfile
import os
import platform

import pywinauto
import pygetwindow

from time import sleep
from enum import Enum
from pynput.keyboard import Controller, KeyCode
from Mouse import ControlMouse


class Macro():
  process_list = list()
  keyboard = Controller()
  UAC_Value = None

  ######키보드 마우스 입력 제어#####
  def BlockInput(self, flag):
    if flag == True:
      ctypes.windll.user32.BlockInput(True)
    elif flag == False:
      ctypes.windll.user32.BlockInput(False)

  ######프로세스 리스트대로 실행######
  def run_process(self, process_list):
    ReturnValue = DefineReturn.NONE
    process_len = len(process_list)
    index = None
    success_cnt = 0
    # option : [UAC 여부, 키보드마우스 제어 여부]
    print(process_list)

    ###### 첫번째 옵션 UAC 켜고 끄기######
    option = process_list[0]
    '''if option[0] == True:
      self.UAC_Value = self.UAC_GetValue()
      self.UAC_ChangeValue(0)'''
    if option[1] == True:
      print("keyboard, mouse control activate")
      ######아래 함수 주석 없애면 마우스, 키보드 입력 받지 않음#######
      #self.BlockInput(True)

    #for proc_cnt, process in enumerate(process_list):
    for index in range(0, process_len):
      # class_name/control_type/child_window
      # app 찾을때까지
      try:
        if index == 1:
          #####프로세스를 만들때 제어판 또는 설정에서 시작해서 만들어야 하도록 되어있음######
          if process_list[index]['app_name'] == "제어판":
            #제어판 실행
            pywinauto.Application(backend='uia').start("control")
          elif process_list[index]['app_name'] == "설정":
            # 윈도우 설정 창 실행
            self.run_ms_settings()

        if index != 0:
          #uia 모드 실행
          if process_list[index]['mode'] == 'uia':
            ReturnValue = self.run_uia(process_list[index])
            if ReturnValue == DefineReturn.SUCCESS:
              success_cnt = success_cnt + 1
            else:
              ReturnValue = DefineReturn.FAIL
              break
          #이미지 인식 실행
          elif process_list[index]['mode'] == 'image':
            ReturnValue = self.run_image(process_list[index])
            if ReturnValue == DefineReturn.SUCCESS:
              success_cnt = success_cnt + 1
            else:
              ReturnValue = DefineReturn.FAIL
              break
              # process['path']
          #현재 uia나 이미지 인식 실행이 아니라면 키보드 입력임.
          else:
            # 컨트롤을 찾아서 행동하는게 아니므로 동기화 수단이 필요함.
            for key in process_list[index]:
              self.keyboard.press(KeyCode.from_vk(key["key_vk"]))
              self.keyboard.release(KeyCode.from_vk(key["key_vk"]))

      except:
        ReturnValue = DefineReturn.FAIL
        break

    '''if option[0] == True:
      self.UAC_ChangeValue(self.UAC_Value[0])'''

    if option[1] == True:
      self.BlockInput(False)

    if success_cnt == process_len - 1:
      ReturnValue = DefineReturn.SUCCESS

    return index, ReturnValue

  def run_uia(self, process):
    ReturnValue = DefineReturn.NONE

    if 'app_name' in process:
      #창이 존재하는지 확인하고 연결
      app = self.app_connect(process['app_name'], 'uia')
      dlg = app[process['app_name']]

      #uniqueid 생성
      #길이가 긴것부터(text, class명이 합쳐진것부터) 찾음.
      uniqueid_list = ast.literal_eval(process['unique_id'])
      uniqueid_list.sort(key=len, reverse = True)

      find_cnt = 0
      find_ctrl = False
      while(find_ctrl is False):
        ctrls = dlg.descendants()

        #컨트롤 찾기.
        for name in uniqueid_list:
          print(ctrls)
          try:
            foundctrl = pywinauto.findbestmatch.find_best_control_matches(name, ctrls)
          except:
            print('컨트롤 매칭 실패 ...{0}'.format(find_ctrl))
            break

          #찾았으면 컨트롤 종류에 따라서 실행
          if len(foundctrl) != 0:
            ctrl = foundctrl[0]

            if(ctrl.friendly_class_name() == "ListItem"):
              #if (ctrl.element_info.parent.friendly_class_name() == "ComboBox"):
              ctrltemp = ctrl.element_info.parent

              if (ctrltemp.class_name == "ComboBox"):
                foundctrl = pywinauto.findbestmatch.find_best_control_matches(ctrltemp.name, ctrls)
                print(foundctrl)
                ctrl = foundctrl[0]

            # 컨트롤 액션 전에 상단으로
            try:
              # ctrl.exists()  # exists가 무조건 있어야함. 얘가 서칭해주는듯
              app.top_window().set_focus()
              ctrl.set_focus()

            except:
              print("control set_focus error")

            # 버튼은 그냥 클릭
            if process['ctrl_type'] in "Button/RadioButton/Hyperlink":
              # 컨트롤이 가능할 때까지 Page Down
              if ctrl.is_visible() == False:
                pywinauto.keyboard.send_keys("{HOME}")
                sleep(0.5)
                while (ctrl.is_visible() == False):
                  pywinauto.keyboard.send_keys("{PGDN}")
                sleep(0.3)
              if ctrl.is_enabled():
                ctrl.click_input(button='left')

            # 꺼져있을때 켜는거 or 켜져있을때 끄는것만 toggle
            elif process['ctrl_type'] in "Toggle/CheckBox":
              if (process['ctrl_action'] == "켜기" and ctrl.get_toggle_state() == 0) \
                  or (process['ctrl_action'] == "끄기" and ctrl.get_toggle_state() == 1):
                if ctrl.is_enabled():
                  ctrl.toggle()

            # 콤보박스는 셀렉
            elif process['ctrl_type'] == "ComboBox":
              try:
                # ctrl.select(process['ctrl_action'])

                ctrl.select(process['ctrl_action_index'])
              except:
                # collapse 에러 발생
                pass
            else:
              # elif type in "ListItem/TreeItem":
              # 컨트롤이 가능할 때까지 Page Down
              if ctrl.is_visible() == False:
                pywinauto.keyboard.send_keys("{HOME}")
                sleep(0.5)
                while (ctrl.is_visible() == False):
                  pywinauto.keyboard.send_keys("{PGDN}")
                sleep(0.3)
              if ctrl.is_enabled():
                if process['ctrl_action'] == "좌클릭":
                  ctrl.click_input(button='left')
                elif process['ctrl_action'] == "더블클릭":
                  ctrl.double_click_input(button='left')
                elif process['ctrl_action'] == "우클릭":
                  ctrl.click_input(button='right')
                elif process['ctrl_action'] == "텍스트입력":
                  print('텍스트입력')
            ReturnValue = DefineReturn.SUCCESS
            find_ctrl = True
            break

          # 1초 대기후 시도
          #sleep(1)

        print(find_ctrl, find_cnt)
        if find_ctrl == False:
          find_cnt = find_cnt + 1
          sleep(2)

        if find_cnt >= 5:
          break

    return ReturnValue

  def run_image(self, process):
    import logging
    import ctypes

    ReturnValue = DefineReturn.NONE
    if 'app_name' in process:
      mouse = ControlMouse()
      bCnt = 0
      while (bCnt <= 5):
        bCnt = bCnt + 1
        try:
          pywinauto.keyboard.send_keys("{HOME}")
          sleep(0.5)

          #이미지 매칭
          x, y = self.match_image(process['image_name'])
          log.info('------------------{0}'.format(process.__str__()))
          while (x == None and y == None):
            pywinauto.keyboard.send_keys("{TAB}")
            pywinauto.keyboard.send_keys("{PGDN}")
            sleep(0.3)
            x, y = self.match_image(process['image_path'])

          sleep(0.3)
          #x, y = self.match_image(process['path'])

          #찾았으면 액션 실행
          if x != None and y != None:
            mouse.setPos(x, y)
            if process['ctrl_action'] == "좌클릭":
              mouse.click()
            elif process['ctrl_action'] == "더블클릭":
              mouse.doubleClick()
            elif process['ctrl_action'] == "우클릭":
              mouse.clickRight()
            ReturnValue = ReturnValue.SUCCESS
            break

        except:
          print("match_image fail")
        sleep(1)
    return ReturnValue

  def run_keyboard(self, process):
    ReturnValue = DefineReturn.NONE
    if 'app_name' in process:
      pass
    return ReturnValue
  # 앱 연결
  def app_connect(self, app_title, backend):
    app = pywinauto.Application(backend=backend)
    try:
      app.connect(title=app_title)

    except pywinauto.findwindows.WindowAmbiguousError:
      wins = pywinauto.findwindows.find_elements(active_only=True, title=app_title)
      app.connect(handle=wins[0].handle)
    except pywinauto.findwindows.ElementNotFoundError:
      pywinauto.timings.wait_until(30, 0.5, lambda: len(pywinauto.findwindows.find_elements(active_only=True, title=app_title)) > 0)
      wins = pywinauto.findwindows.find_elements(active_only=True, title=app_title)
      app.connect(handle=wins[0].handle)

    #창이 여러개일 경우에 하나 빼고 다 끔
    except pywinauto.findwindows.ElementAmbiguousError:
      wins = pywinauto.findwindows.find_elements(title=app_title)
      app.connect(handle=wins[0].handle)
      windows = app.windows()
      for i in range(0, len(windows)):
        if i != len(windows) - 1:
          windows[i].close()
    return app

  # 현재 화면에서 이미지와 같은 곳을 찾아 좌표 반환
  def match_image(self, imagepath):
    img_array = numpy.fromfile(imagepath, numpy.uint8)
    img = cv2.imdecode(img_array, cv2.IMREAD_UNCHANGED)

    hwin = win32gui.GetDesktopWindow()
    width = win32api.GetSystemMetrics(win32con.SM_CXVIRTUALSCREEN)
    height = win32api.GetSystemMetrics(win32con.SM_CYVIRTUALSCREEN)
    left = win32api.GetSystemMetrics(win32con.SM_XVIRTUALSCREEN)
    top = win32api.GetSystemMetrics(win32con.SM_YVIRTUALSCREEN)
    hwindc = win32gui.GetWindowDC(hwin)
    srcdc = win32ui.CreateDCFromHandle(hwindc)
    memdc = srcdc.CreateCompatibleDC()
    bmp = win32ui.CreateBitmap()
    bmp.CreateCompatibleBitmap(srcdc, width, height)
    memdc.SelectObject(bmp)
    memdc.BitBlt((0, 0), (width, height), srcdc, (left, top), win32con.SRCCOPY)

    signedIntsArray = bmp.GetBitmapBits(True)
    template = numpy.fromstring(signedIntsArray, dtype='uint8')
    template.shape = (height, width, 4)

    srcdc.DeleteDC()
    memdc.DeleteDC()
    win32gui.ReleaseDC(hwin, hwindc)
    win32gui.DeleteObject(bmp.GetHandle())

    img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    template = cv2.cvtColor(template, cv2.COLOR_BGR2GRAY)
    # 이미지 템플릿 매칭
    method = eval("cv2.TM_CCOEFF_NORMED")
    res = cv2.matchTemplate(img, template, method)
    min_val, max_val, min_loc, max_loc = cv2.minMaxLoc(res)
    if max_val >= 0.9:
      match_left, match_top = max_loc

      if left < 0:
        match_left = match_left + left
      else:
        match_left = match_left - left

      y_coord = match_top + top + (img.shape[0] / 2)
      x_coord = match_left + (img.shape[1] / 2)

      return x_coord, y_coord

    else:
      print('일치 이미지 탐색 실패')
      return None, None

  def UAC_GetValue(self):
    uac_reg_path = r"SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System"
    key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, uac_reg_path, 0, winreg.KEY_ALL_ACCESS)
    value = winreg.QueryValueEx(key, "ConsentPromptBehaviorAdmin")
    winreg.CloseKey(key)
    return value

  def UAC_ChangeValue(self, value):
    uac_reg_path = r"SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System"
    key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, uac_reg_path, 0, winreg.KEY_ALL_ACCESS)
    winreg.SetValueEx(key, "ConsentPromptBehaviorAdmin", 0 , winreg.REG_DWORD, value)
    winreg.CloseKey(key)

  # 윈도우 설정 창 실행
  def run_ms_settings(self):
    pywinauto.keyboard.send_keys("{VK_LWIN down}i{VK_LWIN up}")
    name = win32gui.GetWindowText(win32gui.GetForegroundWindow())
    dlg = None
    if name == '설정':
      cnt = 0
      while True:
        if cnt == 5:
          break
        try:
          app = pywinauto.Application(backend='uia').connect(title='설정')
          dlg = app['설정']
          break
        except:
          sleep(1)
          cnt = cnt + 1
    else:
      # 실행이 안됐을 경우 재시도
      sleep(1)
      pywinauto.keyboard.send_keys("{VK_LWIN down}i{VK_LWIN up}")
    return dlg

#######결과 종류 정의########
class DefineReturn(Enum):
  FILE_NOT_FOUND = -3
  INSTANCE_ERROR = -2
  FAIL = -1
  SUCCESS = 1
  NONE = 0

def RunStandAlone(argv, key):
  import Cipher

  ######인수 정상 입력 확인#######
  if argv[0] == "-i":
    INSTANCE_NAME = argv[1]
  else:
    return -1, "옵션이 잘못되었습니다.", DefineReturn.INSTANCE_ERROR

  #파일명이 입력 안됐을때
  if len(INSTANCE_NAME) < 1:  # 필수항목 값이 비어있다면
    #print(FILE_NAME, "-i option is mandatory")  # 필수임을 출력
    return -1, "인수에 파일이 없습니다.", DefineReturn.INSTANCE_ERROR
  else:
    ARSCipher = Cipher.AESCipher(key)
    process_list = list()

    if (platform.machine().endswith('64')):
      CurrentDir = "C:/Program Files (x86)"
    else:
      CurrentDir = "C:/Program Files"
    TempDir = "{0}/Temp/Client".format(CurrentDir)
    os.chdir(TempDir)

    print(INSTANCE_NAME)
    zipfile.ZipFile(INSTANCE_NAME).extractall(TempDir)

    #파일을 못찾았을때

    filename = INSTANCE_NAME.replace(".zip",".txt")
    try:
      file = open(filename, 'rb')
    except FileNotFoundError:
      return -1, "파일을 찾지 못했습니다", DefineReturn.FILE_NOT_FOUND

    ########파일 읽기######
    text_list = file.readlines()
    for text in text_list:
      process_str = ARSCipher.decrypt(bytes(text[2:-1]))
      if 'key_vk' in process_str:
        process = eval(process_str)
        process_list.append(process)
      else:
        process_list.append(eval(process_str))
    file.close()
    macro = Macro()

    #####읽어서 실행#####
    index, result = macro.run_process(process_list)

    return index, "{0}".format(process_list[index]["ctrl_name"]), result


if __name__ == "__main__":
  index, strResult, result = RunStandAlone(sys.argv, cipher_key)    #인수 받아서 실행하도록 하는 함수로 실행
  result_dict = dict()
