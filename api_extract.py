import os
from magic import from_file
from pefile import PE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_MODE_64, CsError

def api_extract(filepath, sizelimit):
  file = filepath.split('\\')[-1]
  API_Dict = dict()

  Flag_Magic = False;
  try:
    # File Magic 가져오기
    FileMagic = from_file(filepath)
    Flag_Magic = True
  except:
    print(file + " : File Open Error")

  if Flag_Magic == True:
    # 실행 가능 파일일 경우만
    print(FileMagic)
    if FileMagic.find('executable') != -1:
      error_flag = False
      # PE 분석
      try:
        pe = PE(filepath)
      except:
        error_flag = True
        print("open PE Fail")

      if error_flag == False:
        Data_exist = True
        '''Static Data'''

        IAT = {}
        try:
          for entry in pe.DIRECTORY_ENTRY_IMPORT:
            for imp in entry.imports:
              if imp.name is not None:
                IAT.update({hex(imp.address): imp.name.decode()})
        except:
          return API_Dict
        # print(IAT)

        # Count API
        pe.parse_data_directories()
        Text_Data = b''
        for section in pe.sections:
          # if section.Name == b'.text\x00\x00\x00':
          if b'text' in section.Name or b'CODE' in section.Name:
            Text_Data = pe.get_data(section.VirtualAddress, section.SizeOfRawData)
            # .text 데이터 크기
            size = section.SizeOfRawData
            # .text 데이터 시작하는 지점의 오프셋
            Start_Offset = pe.OPTIONAL_HEADER.ImageBase + section.VirtualAddress
            # .text 데이터 끝나는 지점의 오프셋
            Last_Offset = Start_Offset + size

            # Text 섹션에 데이터가 없거나 Text 섹션 크기가 제한 크기보다 크면 패스
            if Text_Data == "" or size > 1024 * 1024 * sizelimit:
              Data_exist = False
              break

          try:
            # 64비트 파일
            if "PE32+" in FileMagic:
              md = Cs(CS_ARCH_X86, CS_MODE_64)
              start = 0
              if len(Text_Data) > 0:
                while 1:
                  address = 0
                  for i in md.disasm(Text_Data[start:size], Start_Offset + start):
                    address = i.address
                    # API 호출 발견 로직
                    if i.mnemonic == "call" or i.mnemonic == "jmp":
                      if "qword ptr" in i.op_str and "[rip + " in i.op_str:
                        # 호출 주소 추출
                        API_Address = int(i.op_str[i.op_str.index('0x'):i.op_str.index(']')], 16) + i.address + 6
                        API_Address = hex(API_Address)

                        CheckFlag = False
                        if API_Address in IAT:
                          # API count 가져오기
                          if API_Dict.get(IAT[API_Address]):
                            count = API_Dict.get(IAT[API_Address])
                          else:
                            count = 0
                          # API count 증가
                          count = count + 1
                          # API Dict 업데이트
                          API_Dict.update({IAT[API_Address]: count})
                        else:
                          try:
                            int(API_Address, 16)
                            CheckFlag = True
                          except:
                            continue

                        if CheckFlag == True:
                          if hex(int(API_Address, 16) + 1) in IAT:
                            API_Address = hex(int(API_Address, 16) + 1)
                            # API count 가져오기
                            if API_Dict.get(IAT[API_Address]):
                              count = API_Dict.get(IAT[API_Address])
                            else:
                              count = 0
                            # API count 증가
                            count = count + 1
                            # API Dict 업데이트
                            API_Dict.update({IAT[API_Address]: count})

                  # 오프셋 전부다 읽으면
                  if (Last_Offset - address) < 8:
                    break
                  # 못읽었을 경우
                  elif address == 0:
                    start = start + 1
                  else:
                    start = address - (Start_Offset) + 1

            # 32비트 파일
            elif "PE32" in FileMagic:
              md = Cs(CS_ARCH_X86, CS_MODE_32)
              start = 0
              if len(Text_Data) > 0:
                Head_API = None
                API_Sequence = None
                while 1:
                  address = 0
                  j = None
                  for i in md.disasm(Text_Data[start:size], Start_Offset + start):
                    address = i.address
                    # print("0x%x:\t%s\t%s" % (i.address, i.mnemonic, i.op_str))

                    '''if j is not None:
                      print("j : 0x%x:\t%s\t%s" % (j.address, j.mnemonic, j.op_str))
                    print("i : 0x%x:\t%s\t%s" % (i.address, i.mnemonic, i.op_str))'''
                    if i.mnemonic == "call":
                      if "eax" not in i.op_str and \
                          "ecx" not in i.op_str and \
                          "edx" not in i.op_str and \
                          "ebx" not in i.op_str and \
                          "ebp" not in i.op_str and \
                          "esp" not in i.op_str and \
                          "esi" not in i.op_str and \
                          "edi" not in i.op_str:
                        # print("0x%x:\t%s\t%s" % (i.address, i.mnemonic, i.op_str))
                        # print(i.op_str[i.op_str.index('[')+1:i.op_str.index(']')])
                        try:
                          if "+" not in i.op_str:
                            API_Address = i.op_str[i.op_str.index('[') + 1:i.op_str.index(']')]
                            # print(API_Address)
                        except:
                          API_Address = i.op_str
                          # print(API_Address)

                        # print("0x%x:\t%s\t%s" % (i.address, i.mnemonic, i.op_str))
                        # print(i.address
                        # 1. 코드에 바로 해당 주소가 있을때
                        CheckFlag = False
                        if API_Address in IAT:
                          # print(IAT[API_Address])
                          # API count 가져오기
                          if API_Dict.get(IAT[API_Address]):
                            count = API_Dict.get(IAT[API_Address])
                          else:
                            count = 0
                          # API count 증가
                          count = count + 1
                          # API Dict 업데이트
                          API_Dict.update({IAT[API_Address]: count})

                        # 2. 추가 처리가 필요한 경우
                        else:
                          try:
                            int(API_Address, 16)
                            CheckFlag = True
                          except:
                            continue

                          if CheckFlag == True:
                            if hex(int(API_Address, 16) + 1) in IAT:
                              API_Address = hex(int(API_Address, 16) + 1)
                              # API count 가져오기
                              if API_Dict.get(IAT[API_Address]):
                                count = API_Dict.get(IAT[API_Address])
                              else:
                                count = 0
                              # API count 증가
                              count = count + 1
                              # API Dict 업데이트
                              API_Dict.update({IAT[API_Address]: count})

                            # 해당 주소를 찾아가서 op_str확인, jmp일경우 매칭
                            elif (int(API_Address, 16) - pe.OPTIONAL_HEADER.ImageBase) > 0 and (
                                int(API_Address, 16) - pe.OPTIONAL_HEADER.ImageBase) < (
                                pe.OPTIONAL_HEADER.SizeOfCode + pe.OPTIONAL_HEADER.SizeOfInitializedData):

                              try:
                                trace = pe.get_data((int(API_Address, 16) - pe.OPTIONAL_HEADER.ImageBase), 8)
                                for temp_trace in md.disasm(trace, 0):
                                  # print("0x%x:\t%s\t%s" % (j.address, j.mnemonic, j.op_str))
                                  if temp_trace.mnemonic == "jmp":
                                    try:
                                      API_Address = temp_trace.op_str[
                                                    temp_trace.op_str.index('[') + 1:temp_trace.op_str.index(']')]
                                    except:
                                      API_Address = temp_trace.op_str
                                    if API_Address in IAT:
                                      # print(IAT[API_Address])
                                      # API count 가져오기
                                      if API_Dict.get(IAT[API_Address]):
                                        count = API_Dict.get(IAT[API_Address])
                                      else:
                                        count = 0
                                      # API count 증가
                                      count = count + 1
                                      # API Dict 업데이트
                                      API_Dict.update({IAT[API_Address]: count})
                              except:
                                pass

                      elif "eax" in i.op_str:
                        if j is not None and ("lea" in j.op_str or "mov" in j.op_str):
                          API_Address = j.op_str[j.op_str.index('[') + 1:j.op_str.index(']')]
                          if API_Address in IAT:
                            # print(IAT[API_Address])
                            # API count 가져오기
                            if API_Dict.get(IAT[API_Address]):
                              count = API_Dict.get(IAT[API_Address])
                            else:
                              count = 0
                            # API count 증가
                            count = count + 1
                            # API Dict 업데이트
                            API_Dict.update({IAT[API_Address]: count})
                          elif hex(int(API_Address, 16) + 1) in IAT:
                            API_Address = hex(int(API_Address, 16) + 1)
                            # API count 가져오기
                            if API_Dict.get(IAT[API_Address]):
                              count = API_Dict.get(IAT[API_Address])
                            else:
                              count = 0
                            # API count 증가
                            count = count + 1
                            # API Dict 업데이트
                            API_Dict.update({IAT[API_Address]: count})

                    j = i

                  # 오프셋 전부다 읽으면
                  if (Last_Offset - address) < 8:
                    break
                  # 못읽었을 경우
                  elif address == 0:
                    start = start + 1
                  else:
                    start = address - (Start_Offset) + 1
          except CsError as e:
            print("ERROR: %s" % e)

  return API_Dict