from PIL import Image

def crop_and_extract_rgb(image_path):
  """Crops an image into 8x8 sections and extracts RGB values.

  Args:
    image_path: The path to the input image.

  Returns:
    A list of lists, where each inner list contains the RGB values of an 8x8 section.
  """

  image = Image.open(image_path).convert(mode='RGB')
  width, height = 8,8

  # Ensure image dimensions are divisible by 8
  if width % 8 != 0 or height % 8 != 0:
    raise ValueError("Image width and height must be multiples of 8.")

  rgb_values = []

  for y in range(0, height):
    for x in range(0, width):
      section = image.crop((x, y, x + 8, y + 8))
      section_rgb = []
      for pixel_y in range(8):
        for pixel_x in range(8):
          r, g, b = section.getpixel((pixel_x, pixel_y))
          section_rgb.append([r, g, b])
      rgb_values.append(section_rgb)

  return rgb_values

ars = crop_and_extract_rgb("crops.png")
print(ars[0])