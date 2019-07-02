import os
import imageio

png_dir = './'
images = []
for file_name in os.listdir(png_dir):
    if file_name.endswith('.png'):
        file_path = os.path.join(png_dir, file_name)
        print("including file %s\n" % file_name)
        images.append(imageio.imread(file_path))
imageio.mimsave('./CONDX.gif', images)
