#!/usr/bin/env python3
"""Generate deterministic store artwork from real FairWindSK captures."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont


# Resolve every input relative to the repository so the script is relocatable.
REPOSITORY = Path(__file__).resolve().parents[2]
# Keep generated output beside the store metadata reviewed by publishers.
STORES = REPOSITORY / "docs" / "stores"
# Reuse the exact icon shipped in the Apple asset catalog.
ICON_SOURCE = REPOSITORY / "apple" / "Assets.xcassets" / "AppIcon.appiconset" / "AppIcon-1024.png"
# Use the generated abstract background only as a backdrop, never as product UI.
FEATURE_BACKGROUND = STORES / "playstore" / "graphics" / "feature-graphic-background.png"
# Define captures and claims once so every store receives the same story.
CAPTURES = (
    ("launcher-overview.png", "Your helm. One clear workspace.", "La tua postazione. Un ambiente chiaro."),
    ("screen-layout-overview.png", "Live instruments at a glance", "Strumenti in tempo reale"),
    ("mydata-waypoints-list.png", "Your vessel data, organized", "I dati di bordo, organizzati"),
    ("settings-overview.png", "Built for touch and changing light", "Progettato per il touch e la luce"),
    ("pob-bar-active.png", "Safety controls within reach", "Comandi di sicurezza a portata"),
)
# Describe each raster family with its exact upload dimensions.
OUTPUTS = (
    ("appstore", "iphone-6.9-landscape", (2796, 1290)),
    ("appstore", "ipad-13-landscape", (2752, 2064)),
    ("playstore", "phone", (1920, 1080)),
    ("playstore", "tablet", (2560, 1600)),
)


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    """Load a broadly available macOS font with a portable fallback."""
    # Prefer the platform font used by native Apple interfaces.
    suffix = "Bold" if bold else "Regular"
    # Address the installed macOS font explicitly for predictable rendering.
    mac_font = Path(f"/System/Library/Fonts/SFNS.ttf")
    # Use DejaVu when the assets are regenerated on Linux or CI.
    fallback = Path(f"/usr/share/fonts/truetype/dejavu/DejaVuSans{'-Bold' if bold else ''}.ttf")
    # Select the first existing font without introducing another dependency.
    selected = mac_font if mac_font.exists() else fallback
    # Let Pillow fail clearly if neither supported font is installed.
    return ImageFont.truetype(str(selected), size=size)


def cover(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    """Resize and center-crop an image so it completely fills the destination."""
    # Compute the larger scale so neither destination edge remains uncovered.
    scale = max(size[0] / image.width, size[1] / image.height)
    # Round dimensions to stable integers for Pillow.
    resized_size = (round(image.width * scale), round(image.height * scale))
    # Preserve clean app UI edges with high-quality Lanczos resampling.
    resized = image.resize(resized_size, Image.Resampling.LANCZOS)
    # Center the crop so the generated background remains balanced.
    left = (resized.width - size[0]) // 2
    # Center vertically for the same reason.
    top = (resized.height - size[1]) // 2
    # Return the exact target dimensions.
    return resized.crop((left, top, left + size[0], top + size[1]))


def fit(image: Image.Image, bounds: tuple[int, int]) -> Image.Image:
    """Resize an image to fit entirely inside the supplied bounds."""
    # Use the smaller scale so the complete product interface remains visible.
    scale = min(bounds[0] / image.width, bounds[1] / image.height)
    # Convert the proportional dimensions to exact pixels.
    size = (round(image.width * scale), round(image.height * scale))
    # Apply a high-quality filter that preserves small labels.
    return image.resize(size, Image.Resampling.LANCZOS)


def add_shadow(canvas: Image.Image, box: tuple[int, int, int, int], radius: int) -> None:
    """Paint a soft operational-card shadow behind a screenshot."""
    # Create a transparent layer so blur does not affect the base artwork.
    layer = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    # Draw the initial opaque rounded rectangle on the shadow layer.
    ImageDraw.Draw(layer).rounded_rectangle(box, radius=radius, fill=(0, 0, 0, 190))
    # Blur the rectangle into a restrained halo.
    layer = layer.filter(ImageFilter.GaussianBlur(radius // 2))
    # Composite the shadow while preserving an RGB final image.
    canvas.paste(layer, (0, 0), layer)


def make_screenshot(source: Image.Image, title: str, size: tuple[int, int]) -> Image.Image:
    """Build a localized store screenshot around a genuine app capture."""
    # Scale the generated maritime backdrop to the requested store dimensions.
    background = cover(Image.open(FEATURE_BACKGROUND).convert("RGB"), size)
    # Darken the artwork so the real interface and text dominate.
    tint = Image.new("RGB", size, (1, 13, 35))
    # Blend rather than flatten to retain subtle wave and chart texture.
    canvas = Image.blend(background, tint, 0.42)
    # Derive spacing from canvas height so each device family remains balanced.
    margin = round(size[1] * 0.055)
    # Reserve the upper band for one concise localized value proposition.
    headline_height = round(size[1] * 0.19)
    # Fit the full app capture inside the remaining safe area.
    screenshot = fit(source, (size[0] - margin * 2, size[1] - headline_height - margin * 2))
    # Center the interface horizontally.
    screenshot_x = (size[0] - screenshot.width) // 2
    # Place it beneath the headline with stable breathing room.
    screenshot_y = headline_height + margin
    # Describe a rounded card slightly larger than the interface.
    pad = max(8, round(size[1] * 0.012))
    # Calculate the complete card and shadow rectangle.
    box = (
        screenshot_x - pad,
        screenshot_y - pad,
        screenshot_x + screenshot.width + pad,
        screenshot_y + screenshot.height + pad,
    )
    # Add depth without introducing a device frame.
    add_shadow(canvas, box, max(18, round(size[1] * 0.025)))
    # Draw the crisp high-contrast edge used by the marine-MFD visual system.
    draw = ImageDraw.Draw(canvas)
    # Render a subtle blue border around the real screenshot.
    draw.rounded_rectangle(box, radius=max(18, round(size[1] * 0.018)), fill=(17, 38, 67), outline=(74, 154, 255), width=max(2, size[1] // 400))
    # Paste the real capture without obscuring or inventing UI.
    canvas.paste(screenshot, (screenshot_x, screenshot_y))
    # Use a readable helm-distance headline size.
    headline_font = font(max(38, round(size[1] * 0.07)), bold=True)
    # Measure the localized title for precise centering.
    title_box = draw.textbbox((0, 0), title, font=headline_font)
    # Calculate centered text placement.
    title_x = (size[0] - (title_box[2] - title_box[0])) // 2
    # Keep the title inside the safe upper region.
    title_y = max(24, (headline_height - (title_box[3] - title_box[1])) // 2)
    # Paint a small offset shadow for contrast on all comfort colors.
    draw.text((title_x + 3, title_y + 3), title, font=headline_font, fill=(0, 0, 0))
    # Paint the final localized headline.
    draw.text((title_x, title_y), title, font=headline_font, fill=(255, 255, 255))
    # Guarantee an RGB image with no forbidden alpha channel.
    return canvas.convert("RGB")


def make_feature_graphic() -> Image.Image:
    """Compose the Play feature graphic from exact brand assets."""
    # Crop the generated background to Google's required size.
    canvas = cover(Image.open(FEATURE_BACKGROUND).convert("RGB"), (1024, 500))
    # Apply a soft overlay to improve logo and title contrast.
    canvas = Image.blend(canvas, Image.new("RGB", canvas.size, (0, 15, 42)), 0.25)
    # Load the shipping icon in RGB form.
    icon = Image.open(ICON_SOURCE).convert("RGB")
    # Keep the icon substantial while respecting Google's crop-safe area.
    icon = icon.resize((290, 290), Image.Resampling.LANCZOS)
    # Place the icon within the left safe area.
    canvas.paste(icon, (74, 105))
    # Prepare deterministic brand text instead of generated lettering.
    draw = ImageDraw.Draw(canvas)
    # Use a strong, readable product wordmark.
    name_font = font(76, bold=True)
    # Use a concise supporting statement.
    strapline_font = font(31)
    # Render the product name well inside the feature-graphic safe area.
    draw.text((410, 170), "FairWindSK", font=name_font, fill=(255, 255, 255))
    # Render the supporting line without unverified marketing claims.
    draw.text((414, 265), "Marine MFD for Signal K", font=strapline_font, fill=(151, 210, 255))
    # Guarantee the required non-alpha RGB output.
    return canvas.convert("RGB")


def save_png(image: Image.Image, destination: Path) -> None:
    """Save an optimized, non-interlaced PNG after creating its directory."""
    # Ensure a clean checkout can generate the complete directory tree.
    destination.parent.mkdir(parents=True, exist_ok=True)
    # Store lossless UI artwork and strip accidental metadata.
    image.save(destination, format="PNG", optimize=True)


def main() -> None:
    """Generate icons, feature art, and localized screenshots."""
    # Copy the exact Apple catalog icon into the App Store publishing package.
    save_png(Image.open(ICON_SOURCE).convert("RGB"), STORES / "appstore" / "app-icon-1024.png")
    # Resize the same source for Google Play without adding adaptive-icon rounding.
    play_icon = Image.open(ICON_SOURCE).convert("RGB").resize((512, 512), Image.Resampling.LANCZOS)
    # Save the Play Console high-resolution icon.
    save_png(play_icon, STORES / "playstore" / "graphics" / "app-icon-512.png")
    # Build and save the required Play feature graphic.
    save_png(make_feature_graphic(), STORES / "playstore" / "graphics" / "feature-graphic-1024x500.png")
    # Generate every requested device and locale combination.
    for store, device, output_size in OUTPUTS:
        # Select each real UI capture and its localized headline.
        for index, (filename, english_title, italian_title) in enumerate(CAPTURES, start=1):
            # Process both shipped languages in lockstep.
            for locale, title in (("en-US", english_title), ("it-IT", italian_title)):
                # Reuse the localized manual capture when one exists.
                source_path = REPOSITORY / "docs" / "manual" / ("italian" if locale == "it-IT" else "english") / "figures" / filename
                # Load real interface pixels in the required non-alpha color mode.
                source = Image.open(source_path).convert("RGB")
                # Create a stable filename that preserves store ordering.
                destination = STORES / store / "screenshots" / locale / device / f"{index:02d}-{Path(filename).stem}.png"
                # Render the localized store artwork.
                artwork = make_screenshot(source, title, output_size)
                # Save the finished upload-ready asset.
                save_png(artwork, destination)


# Run generation only when the script is invoked directly.
if __name__ == "__main__":
    # Enter the deterministic asset pipeline.
    main()

