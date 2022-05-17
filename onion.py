import sys
import os
import urllib.request 
import ssl
import subprocess

ONION_REPO = 'https://github.com/BareMetalEngine/onion.git'
ONION_ARTIFACT_WINDOWS = 'https://github.com/BareMetalEngine/onion/releases/latest/download/onion.exe'
ONION_ARTIFACT_LINUX = 'https://github.com/BareMetalEngine/onion/releases/latest/download/onion'

def make_directory(path):
    try:
        if not os.path.exists(path):
            print("Creating directory: '%s'" % path)
            os.makedirs(path)
    except Exception as e:
        print("Unable to create directory: ", path)
        print(e)
        sys.exit(-1)

def find_onion_binary():
    root_path = os.path.abspath(__file__)

    if sys.platform == 'win32':
        binary_name = 'onion.exe'
        artifact_name = ONION_ARTIFACT_WINDOWS
    else:
        binary_name = 'onion'
        artifact_name = ONION_ARTIFACT_LINUX

    onion_path = os.path.join(os.path.dirname(root_path), ".bin", binary_name)

    if os.path.isfile(onion_path):
        return onion_path

    make_directory(os.path.dirname(onion_path))

    ssl._create_default_https_context = ssl._create_unverified_context
    urllib.request.urlretrieve(artifact_name, onion_path)
    
    return onion_path

def main():
    onion_path = find_onion_binary()
    if not os.path.isfile(onion_path):
        print("Unable to find onion binary")
        sys.exit(-3)

    print("Found Onion at '{path}'".format(path=onion_path))

    foward_cmd = [onion_path] + sys.argv[1:]
    subprocess.call(foward_cmd)

    return


if __name__ == "__main__":
    main()