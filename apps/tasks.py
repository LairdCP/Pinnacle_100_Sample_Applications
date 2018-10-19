import sys
import os
import shutil
import subprocess

TASK_CMAKE = "cmake"
TASK_BUILD = "build"
TASK_FLASH = "flash"
TASK_CLEAN = "clean"


def cmakeTask(app):
    print("cmake " + app)
    if (os.path.exists(app)):
        shutil.rmtree(app)
    os.mkdir(app)
    os.chdir("../zephyr")
    #subprocess.check_call("source ./zephyr-env.sh")
    os.chdir("../build/{}".format(app))
    subprocess.check_call("cmake -GNinja ../../apps/{0}".format(app), env=os.environ)

def buildTask(app):
    print("build " + app)


def flashTask(app):
    print("flash " + app)


def cleanTask(app):
    print("flash " + app)


def checkTasks():
    task = sys.argv[1]
    appName = sys.argv[2]
    if task == TASK_CMAKE:
        cmakeTask(appName)
    elif task == TASK_BUILD:
        buildTask(appName)
    elif task == TASK_FLASH:
        flashTask(appName)
    elif task == TASK_CLEAN:
        cleanTask(appName)
    else:
        exit ("Unknown task {}".format(task))


def main():
    numArgs = len(sys.argv)
    if numArgs != 3:
        exit("Incorrect amount of args: {}".format(numArgs))
    checkTasks()

main()