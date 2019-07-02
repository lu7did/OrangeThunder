import os
import imageio

png_dir = './'
images = []
for file_name in sorted(os.listdir(png_dir)):
#for file_name in os.listdir(png_dir):
    if file_name.endswith('.png'):
        file_path = os.path.join(png_dir, file_name)
        print("makegif: including file %s\n" % file_path)
        images.append(imageio.imread(file_path))
imageio.mimsave('./CONDX.gif', images, duration=0.05)
