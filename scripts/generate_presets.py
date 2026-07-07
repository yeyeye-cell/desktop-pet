"""Generate preset pet sprite frames for DesktopPet."""
import json
import math
from pathlib import Path
from PIL import Image, ImageDraw

SIZE = 128
OUT = Path("resources/presets")

def circle_bbox(cx, cy, r):
    return (cx - r, cy - r, cx + r, cy + r)

def draw_cat(frame: int, state: str) -> Image.Image:
    """Orange tabby cat."""
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Body
    body_y = 55 + int(2 * math.sin(frame * 0.5))
    d.ellipse(circle_bbox(64, body_y + 15, 28), fill=(255, 153, 51, 255))

    # Head
    head_y = body_y - 10
    d.ellipse(circle_bbox(64, head_y, 22), fill=(255, 153, 51, 255))

    # Ears (triangles)
    for ex in [46, 82]:
        d.polygon([(ex - 10, head_y - 15), (ex, head_y - 35), (ex + 10, head_y - 15)],
                  fill=(255, 153, 51, 255))
        d.polygon([(ex - 6, head_y - 15), (ex, head_y - 30), (ex + 6, head_y - 15)],
                  fill=(255, 200, 150, 255))

    # Eyes
    eye_y = head_y - 4
    blink = 1.0
    if state == "sleep":
        blink = 0.2
    elif state == "happy":
        blink = 1.2
    for ex in [56, 72]:
        d.ellipse(circle_bbox(ex, eye_y, 5), fill=(255, 255, 255, 255))
        d.ellipse(circle_bbox(ex, eye_y, int(3 * blink)), fill=(30, 30, 30, 255))

    # Nose & mouth
    d.ellipse(circle_bbox(64, eye_y + 8, 2), fill=(255, 100, 100, 255))
    if state == "happy":
        d.arc([58, eye_y + 4, 70, eye_y + 16], 0, 180, fill=(100, 60, 30, 255), width=2)
    elif state == "clicked":
        d.ellipse(circle_bbox(64, eye_y + 10, 4), fill=(60, 30, 30, 255))

    # Whiskers
    for wx in [46, 82]:
        for wy in [eye_y + 3, eye_y + 7]:
            d.line([(wx - 12, wy), (wx + 12, wy)], fill=(200, 200, 200, 180), width=1)

    # Tail
    tail_sway = int(8 * math.sin(frame * 1.2))
    tail_y = body_y + 10
    if state == "happy":
        tail_sway = int(15 * math.sin(frame * 0.8))
    d.line([(90, tail_y), (100 + tail_sway, tail_y - 15), (108, tail_y - 25)],
           fill=(255, 153, 51, 255), width=5)

    # Paws
    paw_offset = int(3 * math.sin(frame * 0.7))
    for px in [48, 64, 80]:
        d.ellipse(circle_bbox(px, body_y + 38 + paw_offset, 6), fill=(255, 180, 100, 255))

    # Sleep: Zzz
    if state == "sleep":
        d.text((100, 8 + int(frame % 2) * 4), "Z", fill=(180, 200, 255, 220))

    return img


def draw_dog(frame: int, state: str) -> Image.Image:
    """Shiba Inu style dog."""
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    body_y = 58 + int(2 * math.sin(frame * 0.4))
    # Body
    d.ellipse(circle_bbox(64, body_y + 15, 30), fill=(210, 160, 100, 255))
    # Belly
    d.ellipse(circle_bbox(64, body_y + 20, 18), fill=(255, 225, 190, 255))

    # Head
    head_y = body_y - 8
    d.ellipse(circle_bbox(64, head_y, 24), fill=(210, 160, 100, 255))
    # Snout
    d.ellipse(circle_bbox(64, head_y + 8, 12), fill=(255, 225, 190, 255))

    # Ears (floppy triangles)
    for ex, dir_x in [(48, -1), (80, 1)]:
        d.polygon([(ex, head_y - 20), (ex - 6 * dir_x, head_y - 38), (ex - 14 * dir_x, head_y - 18)],
                  fill=(160, 110, 60, 255))

    # Eyes
    eye_y = head_y - 5
    blink = 1.0
    if state == "sleep":
        blink = 0.15
    elif state == "clicked":
        blink = 1.3
    for ex in [54, 74]:
        d.ellipse(circle_bbox(ex, eye_y, 4), fill=(30, 30, 30, 255))
        d.ellipse(circle_bbox(ex, eye_y, int(3.5 * blink)), fill=(255, 255, 255, 255))

    # Nose
    d.ellipse(circle_bbox(64, eye_y + 6, 3), fill=(40, 40, 40, 255))

    # Mouth
    mouth_y = eye_y + 10
    if state == "happy":
        d.arc([54, mouth_y - 2, 74, mouth_y + 14], 0, 180, fill=(100, 60, 30, 255), width=2)
        # Tongue
        d.ellipse(circle_bbox(64, mouth_y + 8, 4), fill=(255, 150, 150, 255))

    # Tail (curved up)
    tail_sway = int(5 * math.sin(frame * 1.0))
    tail_points = [(94, body_y), (100 + tail_sway, body_y - 15), (96 + tail_sway, body_y - 30)]
    d.line(tail_points, fill=(210, 160, 100, 255), width=6)

    # Legs
    leg_offset = int(2 * math.sin(frame * 0.6))
    for px in [46, 82]:
        d.ellipse(circle_bbox(px, body_y + 36 + leg_offset, 7), fill=(190, 140, 80, 255))

    if state == "sleep":
        d.text((100, 8 + int(frame % 2) * 3), "z", fill=(180, 200, 255, 200))

    return img


def draw_bird(frame: int, state: str) -> Image.Image:
    """Blue bird."""
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    bounce = int(4 * math.sin(frame * 0.8))
    body_cx, body_cy = 64, 55 + bounce

    # Body
    d.ellipse(circle_bbox(body_cx, body_cy, 22), fill=(80, 180, 230, 255))
    # Belly
    d.ellipse(circle_bbox(body_cx, body_cy + 5, 14), fill=(200, 230, 255, 255))

    # Head
    head_y = body_cy - 22
    d.ellipse(circle_bbox(body_cx, head_y, 14), fill=(80, 180, 230, 255))

    # Beak
    beak_open = 0
    if state == "happy" or state == "clicked":
        beak_open = 4
    d.polygon([(78, head_y - 2), (94, head_y), (78, head_y + 4 + beak_open)],
              fill=(255, 180, 50, 255))

    # Eyes
    eye_y = head_y - 2
    eye_open = 1.0
    if state == "sleep":
        eye_open = 0.15
    d.ellipse(circle_bbox(body_cx + 6, eye_y, 5), fill=(255, 255, 255, 255))
    d.ellipse(circle_bbox(body_cx + 6, eye_y, int(3 * eye_open)), fill=(20, 20, 20, 255))

    # Wing
    wing_y = body_cy - 5
    if state == "walk":
        # Flap
        d.ellipse([38, wing_y - 15, 68, wing_y + 10], fill=(60, 150, 210, 255))
    elif state == "happy":
        d.ellipse([36, wing_y - 20, 66, wing_y + 8], fill=(50, 140, 200, 255))
    else:
        d.ellipse([42, wing_y - 5, 66, wing_y + 10], fill=(60, 150, 210, 255))

    # Tail feathers
    tail_y = body_cy + 10
    for i in range(3):
        tx = 40 + i * 6
        d.line([(tx, tail_y), (tx - 8, tail_y + 18)], fill=(50, 150, 220, 255), width=3)

    # Feet
    foot_y = body_cy + 20
    for fx in [56, 72]:
        d.line([(fx, foot_y), (fx - 4, foot_y + 8), (fx + 4, foot_y + 8)],
               fill=(255, 180, 50, 255), width=2)

    if state == "sleep":
        d.text((95, 8 + int(frame % 2) * 4), "Z", fill=(180, 200, 255, 200))

    return img


PETS = {
    "cat":  {"name": "小橘猫", "draw": draw_cat,  "frameRate": 120},
    "dog":  {"name": "柴犬",   "draw": draw_dog,  "frameRate": 120},
    "bird": {"name": "小蓝鸟", "draw": draw_bird, "frameRate": 100},
}

STATES = {
    "idle":  4,
    "walk":  4,
    "click": 2,
    "happy": 2,
    "sleep": 2,
}


def main():
    OUT.mkdir(parents=True, exist_ok=True)

    for pet_key, pet_info in PETS.items():
        pet_dir = OUT / pet_key
        pet_dir.mkdir(exist_ok=True)

        # Write pet.json
        with open(pet_dir / "pet.json", "w") as f:
            json.dump({"name": pet_info["name"], "frameRate": pet_info["frameRate"]}, f)

        for state, num_frames in STATES.items():
            state_dir = pet_dir / state
            state_dir.mkdir(exist_ok=True)

            for i in range(num_frames):
                img = pet_info["draw"](i, state)
                # Apply scaling
                img.save(state_dir / f"{state}_{i+1:03d}.png")

        print(f"Generated {pet_info['name']} ({pet_key})")


if __name__ == "__main__":
    main()
