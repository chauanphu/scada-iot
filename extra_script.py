import shutil
import os

def move_bin_file(source, target, env):
    # Define the path to the .bin file that was built
    firmware_bin = os.path.join(env.subst("$BUILD_DIR"), "firmware.bin")
    # Define the destination directory (local /bin folder)
    bin_folder = os.path.join(env.subst("$PROJECT_DIR"), "bin")
    
    # Create the bin folder if it doesn't exist
    if not os.path.exists(bin_folder):
        os.makedirs(bin_folder)
    
    # Move the .bin file to the bin folder
    shutil.move(firmware_bin, os.path.join(bin_folder, "firmware.bin"))
