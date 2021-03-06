// license:BSD-3-Clause
// copyright-holders:AJR
/****************************************************************************

    Skeleton driver for Roland S-10 and related samplers.

    MKS-100 is S-10 without the keyboard; the two use the same main board.
    S-220 also lacks a keyboard interface but implements a four-channel
    VCA.

****************************************************************************/

#include "emu.h"
//#include "bus/midi/midi.h"
#include "cpu/mcs51/mcs51.h"
#include "machine/i8251.h"
#include "machine/mb63h149.h"
#include "machine/nvram.h"
#include "machine/rescap.h"
#include "machine/upd7001.h"
#include "video/hd44780.h"
#include "emupal.h"
#include "screen.h"

class roland_s10_state : public driver_device
{
public:
	roland_s10_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_usart(*this, "usart")
		, m_lcdc(*this, "lcdc")
	{
	}

	void s10(machine_config &config);
	void mks100(machine_config &config);

protected:
	HD44780_PIXEL_UPDATE(lcd_pixel_update);

	u8 qdd_r(offs_t offset);
	void qdd_w(offs_t offset, u8 data);
	void lcd_data_w(offs_t offset, u8 data);
	void led_data_w(offs_t offset, u8 data);
	u8 sw_scan_r(offs_t offset);
	void sw_scan_w(offs_t offset, u8 data);
	void led_latch_w(u8 data);

	void prog_map(address_map &map);
	void s10_ext_map(address_map &map);
	void mks100_ext_map(address_map &map);

	void palette_init(palette_device &palette);

	required_device<mcs51_cpu_device> m_maincpu;
	required_device<i8251_device> m_usart;
	required_device<hd44780_device> m_lcdc;
};

class roland_s220_state : public roland_s10_state
{
public:
	roland_s220_state(const machine_config &mconfig, device_type type, const char *tag)
		: roland_s10_state(mconfig, type, tag)
	{
	}

	void s220(machine_config &config);

private:
	HD44780_PIXEL_UPDATE(lcd_pixel_update);

	void output_control_w(offs_t offset, u8 data);
	void vca_cv_w(offs_t offset, u8 data);
	void led_latch1_w(u8 data);
	void led_latch2_w(u8 data);

	void s220_ext_map(address_map &map);
};


HD44780_PIXEL_UPDATE(roland_s10_state::lcd_pixel_update)
{
	if (x < 5 && y < 8 && line < 2 && pos < 8)
		bitmap.pix16(y, (line * 8 + pos) * 6 + x) = state;
}

HD44780_PIXEL_UPDATE(roland_s220_state::lcd_pixel_update)
{
	if (x < 5 && y < 8 && line < 2 && pos < 16)
		bitmap.pix16(line * 8 + y, pos * 6 + x) = state;
}

u8 roland_s10_state::qdd_r(offs_t offset)
{
	if (!BIT(offset, 1))
		return m_usart->read(offset);

	return 0;
}

void roland_s10_state::qdd_w(offs_t offset, u8 data)
{
	if (!BIT(offset, 1))
		m_usart->write(offset, data);
}

void roland_s10_state::lcd_data_w(offs_t offset, u8 data)
{
	if (offset == 0)
		m_lcdc->control_w(data);
	else
		m_lcdc->data_w(data);
}

void roland_s10_state::led_data_w(offs_t offset, u8 data)
{
}

u8 roland_s10_state::sw_scan_r(offs_t offset)
{
	return 0;
}

void roland_s10_state::sw_scan_w(offs_t offset, u8 data)
{
}

void roland_s10_state::led_latch_w(u8 data)
{
}

void roland_s220_state::output_control_w(offs_t offset, u8 data)
{
}

void roland_s220_state::vca_cv_w(offs_t offset, u8 data)
{
}

void roland_s220_state::led_latch1_w(u8 data)
{
}

void roland_s220_state::led_latch2_w(u8 data)
{
}

void roland_s10_state::prog_map(address_map &map)
{
	map(0x0000, 0xffff).rom().region("program", 0);
}

void roland_s10_state::mks100_ext_map(address_map &map)
{
	map(0x4000, 0x4003).mirror(0xffc).rw(FUNC(roland_s10_state::qdd_r), FUNC(roland_s10_state::qdd_w));
	map(0x6000, 0x7fff).ram().share("nvram");
	map(0x8000, 0x80ff).mirror(0xf00).w(FUNC(roland_s10_state::lcd_data_w));
	map(0x9000, 0x90ff).mirror(0xf00).w(FUNC(roland_s10_state::led_data_w));
	map(0xa000, 0xa0ff).mirror(0xf00).rw(FUNC(roland_s10_state::sw_scan_r), FUNC(roland_s10_state::sw_scan_w));
	map(0xc000, 0xc000).mirror(0xfff).w(FUNC(roland_s10_state::led_latch_w));
	//map(0xe000, 0xffff).rw("wave", FUNC(rf5c36_device::read), FUNC(rf5c36_device::write));
}

void roland_s10_state::s10_ext_map(address_map &map)
{
	mks100_ext_map(map);
	map(0x5000, 0x57ff).mirror(0x800).rw("keyscan", FUNC(mb63h149_device::read), FUNC(mb63h149_device::write));
	//map(0xb000, 0xb000).mirror(0xfff).rw(FUNC(roland_s10_state::upd7001_r), FUNC(mb63h149_device::upd7001_w));
}

void roland_s220_state::s220_ext_map(address_map &map)
{
	map(0x0000, 0x000f).mirror(0x3ff0).w(FUNC(roland_s220_state::output_control_w));
	map(0x4000, 0x4003).mirror(0xffc).rw(FUNC(roland_s220_state::qdd_r), FUNC(roland_s220_state::qdd_w));
	map(0x5000, 0x5000).mirror(0xfff).w(FUNC(roland_s220_state::led_latch1_w));
	map(0x6000, 0x7fff).ram().share("nvram");
	map(0x8000, 0x80ff).mirror(0xf00).w(FUNC(roland_s220_state::lcd_data_w));
	map(0x9000, 0x90ff).mirror(0xf00).w(FUNC(roland_s220_state::vca_cv_w));
	map(0xa000, 0xa0ff).mirror(0xf00).rw(FUNC(roland_s220_state::sw_scan_r), FUNC(roland_s220_state::sw_scan_w));
	map(0xc000, 0xc000).mirror(0xfff).w(FUNC(roland_s220_state::led_latch2_w));
	//map(0xe000, 0xffff).rw("wave", FUNC(rf5c36_device::read), FUNC(rf5c36_device::write));
}


static INPUT_PORTS_START(s10)
INPUT_PORTS_END

static INPUT_PORTS_START(mks100)
INPUT_PORTS_END

static INPUT_PORTS_START(s220)
INPUT_PORTS_END

void roland_s10_state::palette_init(palette_device &palette)
{
	palette.set_pen_color(0, rgb_t(131, 136, 139));
	palette.set_pen_color(1, rgb_t( 92,  83,  88));
}

void roland_s10_state::s10(machine_config &config)
{
	I8032(config, m_maincpu, 12_MHz_XTAL); // SAB8032A
	m_maincpu->set_addrmap(AS_PROGRAM, &roland_s10_state::prog_map);
	m_maincpu->set_addrmap(AS_IO, &roland_s10_state::s10_ext_map);

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0); // TC5564PL-20 + battery

	I8251(config, m_usart, 6.5_MHz_XTAL / 2); // MB89251A

	//MB87013(config, "qddia", 6.5_MHz_XTAL);

	mb63h149_device &keyscan(MB63H149(config, "keyscan", 12_MHz_XTAL));
	keyscan.int_callback().set_inputline(m_maincpu, MCS51_T1_LINE);

	// LCD unit: LM16155C
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_LCD));
	screen.set_refresh_hz(60);
	screen.set_vblank_time(ATTOSECONDS_IN_USEC(2500)); /* not accurate */
	screen.set_screen_update("lcdc", FUNC(hd44780_device::screen_update));
	screen.set_size(6*16, 8*1);
	screen.set_visarea_full();
	screen.set_palette("palette");

	PALETTE(config, "palette", FUNC(roland_s10_state::palette_init), 2);

	HD44780(config, m_lcdc, 0);
	m_lcdc->set_lcd_size(2, 8);
	m_lcdc->set_pixel_update_cb(FUNC(roland_s10_state::lcd_pixel_update));
	m_lcdc->set_busy_factor(0.005);

	UPD7001(config, "adc", RES_K(27), CAP_P(47));

	//RF5C36(config, "wave", 26.88_MHz_XTAL);
}

void roland_s10_state::mks100(machine_config &config)
{
	s10(config);
	m_maincpu->set_addrmap(AS_IO, &roland_s10_state::mks100_ext_map);

	config.device_remove("keyscan");
	config.device_remove("adc");
}

void roland_s220_state::s220(machine_config &config)
{
	s10(config);
	m_maincpu->set_addrmap(AS_IO, &roland_s220_state::s220_ext_map);

	config.device_remove("keyscan");
	config.device_remove("adc");

	// LCD unit: LDS7A1681A
	subdevice<screen_device>("screen")->set_size(6*16, 8*2);
	subdevice<screen_device>("screen")->set_visarea_full();
	m_lcdc->set_pixel_update_cb(FUNC(roland_s220_state::lcd_pixel_update));
}


ROM_START(s10)
	ROM_REGION(0x10000, "program", 0)
	ROM_LOAD("s-10_roland_2-0-7.ic26", 0x00000, 0x10000, CRC(5e588042) SHA1(a41e626bce036bcc9699bede3af137c2888ac704))
ROM_END

ROM_START(mks100)
	ROM_REGION(0x10000, "program", 0)
	ROM_LOAD("roland_mks-100_v1.04_ic26.bin", 0x00000, 0x10000, CRC(39a94481) SHA1(8c6e84d3298f44512d36fe57b80c8f6ea050197c))
ROM_END

ROM_START(s220)
	ROM_REGION(0x10000, "program", 0)
	ROM_LOAD("roland_s-220_v1.04_ic25.bin", 0x00000, 0x10000, CRC(1b74b694) SHA1(11ce4b47abe48116eb34d575e3da46387240c2b1))
ROM_END

SYST(1986, s10,    0,   0, s10,    s10,    roland_s10_state,  empty_init, "Roland", "S-10 Digital Sampling Keyboard", MACHINE_IS_SKELETON)
SYST(1987, mks100, s10, 0, mks100, mks100, roland_s10_state,  empty_init, "Roland", "MKS-100 Digital Sampler",        MACHINE_IS_SKELETON)
SYST(1987, s220,   0,   0, s220,   s220,   roland_s220_state, empty_init, "Roland", "S-220 Digital Sampler",          MACHINE_IS_SKELETON)
