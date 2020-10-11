#include "../base.hpp"
VRAMViewer *vramViewer = nullptr;

VRAMViewer::VRAMViewer() {
  setTitle("VRAM Viewer");
  setStatusFont(application->proportionalFontBold);
  setStatusVisible();

  layout.setMargin(5);
  modeLabel.setText("Mode:");
  modeSelection.append("2bpp", "4bpp", "8bpp");
  addr.setFont(application->monospaceFont);
  cleantile.setText("Clean");
  dump.setText("Dump");
  autoUpdate.setText("Auto");
  autoUpdate.setChecked(true);
  update.setText("Update");
  canvas.setSize({512, 512});

  layout.append(controlLayout, {~0, 0}, 5);
    controlLayout.append(modeLabel, {0, 0}, 5);
    controlLayout.append(modeSelection, {0, 0}, 5);
    controlLayout.append(spacer, {~0, 0});
    controlLayout.append(addr, {50, 0}, 5);
    controlLayout.append(cleantile, {0, 0}, 5);
    controlLayout.append(dump, {0, 0}, 5);
    controlLayout.append(autoUpdate, {0, 0}, 5);
    controlLayout.append(update, {80, 0});
  layout.append(canvas, {512, 512});
  append(layout);

  modeSelection.onChange = update.onActivate = { &VRAMViewer::updateTiles, this };

  cleantile.onActivate = { &VRAMViewer::cleanTile, this };
  dump.onActivate = { &VRAMViewer::dumpVram, this };

  canvas.onMouseLeave = [&] { setStatusText(""); };
  canvas.onMouseMove = [&](Position position) {
    unsigned x = position.x, y = position.y, mode = modeSelection.selection();
    if(x >= 256 && mode >= 2) return setStatusText("");
    if(y >= 256 && mode >= 1) return setStatusText("");
    string output = { x, ", ", y, ", " };
    x /= 8, y /= 8;
    unsigned tile = 0;
    if(mode == 0) tile = y * 64 + x;
    if(mode == 1) tile = y * 64 + x;
    if(mode == 2) tile = y * 32 + x;

    unsigned address = tile * (16 << mode);
    string data;
	for(unsigned i = 0; i < 0x10; i++) data.append(hex<2>(SNES::ppu.vram[address + i]));

    output.append("Tile: 0x", hex<4>(tile), ", Address: 0x" , hex<4>(address), ", Data: ", data);
    setStatusText(output);
  };

  setGeometry({{128, 128}, layout.minimumGeometry().size()});
  windowManager->append(this, "VRAMViewer");
}

void VRAMViewer::cleanTile() {
	unsigned address = hex(addr.text());

	for(unsigned i = 0; i < 0x10; i++) SNES::ppu.vram[address + i] = 0x0;
}

void VRAMViewer::dumpVram() {
	string filename;
	file fp;
	file fpoam;
	file fpcgram;

	debugger->print("Dumping graphics.\n");

	filename = { interface->pathName, "debug/memory-" };
	filename.append("vram.bin");
	if(fp.open(filename, file::mode::write) == false) return;
	for(unsigned addr = 0; addr <= 0xffff; addr++) fp.write(SNES::ppu.vram[addr]);

	filename = { interface->pathName, "debug/memory-" };
	filename.append("oam.bin");
	if(fpoam.open(filename, file::mode::write) == false) return;
	for(unsigned addr = 0; addr <= 0x021f; addr++) fpoam.write(SNES::ppu.oam[addr]);

	filename = { interface->pathName, "debug/memory-" };
	filename.append("cgram.bin");
	if(fpcgram.open(filename, file::mode::write) == false) return;
	for(unsigned addr = 0; addr <= 0x01ff; addr++) fpcgram.write(SNES::ppu.cgram[addr]);

	debugger->print("Graphics Data Dumped.\n");
}

void VRAMViewer::updateTiles() {
  uint32_t *dp = canvas.data();
  for(unsigned y = 0; y < 512; y++) {
    for(unsigned x = 0; x < 512; x++) {
      *dp++ = 0xff800000;
    }
  }
  dp = canvas.data();
  const uint8_t *sp = SNES::ppu.vram;

  if(modeSelection.selection() == 0) {
    for(unsigned tileY = 0; tileY < 64; tileY++) {
      for(unsigned tileX = 0; tileX < 64; tileX++) {
        for(unsigned y = 0; y < 8; y++) {
          uint8_t d[] = { sp[0], sp[1] };
          for(unsigned x = 0; x < 8; x++) {
            unsigned color = 0;
            color += d[0] & 0x80 ? 1 : 0;
            color += d[1] & 0x80 ? 2 : 0;
            for(auto &b : d) b <<= 1;
            color *= 0x55;
            color = (255u << 24) + (color << 16) + (color << 8) + (color << 0);
            dp[(tileY * 8 + y) * 512 + (tileX * 8 + x)] = color;
          }
          sp += 2;
        }
      }
    }
  }

  if(modeSelection.selection() == 1) {
    for(unsigned tileY = 0; tileY < 32; tileY++) {
      for(unsigned tileX = 0; tileX < 64; tileX++) {
        for(unsigned y = 0; y < 8; y++) {
          uint8_t d[] = { sp[0], sp[1], sp[16], sp[17] };
          for(unsigned x = 0; x < 8; x++) {
            unsigned color = 0;
            color += d[0] & 0x80 ? 1 : 0;
            color += d[1] & 0x80 ? 2 : 0;
            color += d[2] & 0x80 ? 4 : 0;
            color += d[3] & 0x80 ? 8 : 0;
            for(auto &b : d) b <<= 1;
            color *= 0x11;
            color = (255u << 24) + (color << 16) + (color << 8) + (color << 0);
            dp[(tileY * 8 + y) * 512 + (tileX * 8 + x)] = color;
          }
          sp += 2;
        }
        sp += 16;
      }
    }
  }

  if(modeSelection.selection() == 2) {
    for(unsigned tileY = 0; tileY < 32; tileY++) {
      for(unsigned tileX = 0; tileX < 32; tileX++) {
        for(unsigned y = 0; y < 8; y++) {
          uint8_t d[] = { sp[0], sp[1], sp[16], sp[17], sp[32], sp[33], sp[48], sp[49] };
          for(unsigned x = 0; x < 8; x++) {
            unsigned color = 0;
            color += d[0] & 0x80 ?   1 : 0;
            color += d[1] & 0x80 ?   2 : 0;
            color += d[2] & 0x80 ?   4 : 0;
            color += d[3] & 0x80 ?   8 : 0;
            color += d[4] & 0x80 ?  16 : 0;
            color += d[5] & 0x80 ?  32 : 0;
            color += d[6] & 0x80 ?  64 : 0;
            color += d[7] & 0x80 ? 128 : 0;
            for(auto &b : d) b <<= 1;
            color = (255u << 24) + (color << 16) + (color << 8) + (color << 0);
            dp[(tileY * 8 + y) * 512 + (tileX * 8 + x)] = color;
          }
          sp += 2;
        }
        sp += 48;
      }
    }
  }

  canvas.update();
}
