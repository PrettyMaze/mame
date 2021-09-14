// license:BSD-3-Clause
// copyright-holders:Robbbert
/****************************************************************************************
    PINBALL
    Mr. Game 1B11188/0

    These games have a M68000 and 3x Z80, and a M114 Sound IC.
    They have a video screen upon which the scores and other info is displayed.

How to set up the machine (motor show, dakar, wcup90):
- These machines need to be loaded with default settings before they can accept coins
- Press - key (minus in main keyboard)
- Press again until you see test 25 (Motor Show) or test 23 (Dakar)
- In the dipswitch menu turn off the Ram Protect switch
- Press Left shift and Right shift together (game stops responding)
- Turn the Ram Protect Switch back on
- Press F3 or reboot
- The default settings have been saved to nvram and you can insert coins
- However, the game cannot be played due to missing balls.

ToDo:
- Video
- Outputs
- Inputs
- Support for electronic volume control
- Audio rom banking
- Most sounds missing due to unemulated M114 chip

*****************************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/74259.h"
#include "machine/i8255.h"
#include "machine/nvram.h"
#include "machine/timer.h"
//#include "machine/watchdog.h"
#include "sound/dac.h"
#include "sound/tms5220.h"
#include "video/resnet.h"
#include "emupal.h"
#include "screen.h"
#include "speaker.h"

namespace {

class mrgame_state : public driver_device
{
public:
	mrgame_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_palette(*this, "palette")
		, m_p_videoram(*this, "videoram")
		, m_p_objectram(*this, "objectram")
		, m_gfxdecode(*this, "gfxdecode")
		, m_maincpu(*this, "maincpu")
		, m_audiocpu1(*this, "audiocpu1")
		, m_audiocpu2(*this, "audiocpu2")
		, m_videocpu(*this, "videocpu")
		, m_selectlatch(*this, "selectlatch")
		, m_io_dsw0(*this, "DSW0")
		, m_io_dsw1(*this, "DSW1")
		, m_io_x0(*this, "X0")
		, m_io_x1(*this, "X1")
	{ }

	void mrgame(machine_config &config);
	void wcup90(machine_config &config);

	void init_mrgame();

protected:
	virtual void machine_start() override;
	virtual void machine_reset() override;

private:
	void mrgame_palette(palette_device &palette) const;
	void ack1_w(uint8_t data);
	void ack2_w(uint8_t data);
	void portb_w(uint8_t data);
	void row_w(uint8_t data);
	void sound_w(uint8_t data);
	void triple_w(uint8_t data);
	void video_w(uint8_t data);
	DECLARE_WRITE_LINE_MEMBER(video_a11_w);
	DECLARE_WRITE_LINE_MEMBER(video_a12_w);
	DECLARE_WRITE_LINE_MEMBER(video_a13_w);
	DECLARE_WRITE_LINE_MEMBER(intst_w);
	DECLARE_WRITE_LINE_MEMBER(nmi_intst_w);
	DECLARE_WRITE_LINE_MEMBER(flip_w);
	uint8_t col_r();
	uint8_t sound_r();
	uint8_t porta_r();
	uint8_t portc_r();
	uint8_t rsw_r();
	DECLARE_WRITE_LINE_MEMBER(vblank_int_w);
	DECLARE_WRITE_LINE_MEMBER(vblank_nmi_w);
	TIMER_DEVICE_CALLBACK_MEMBER(irq_timer);
	uint32_t screen_update_mrgame(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void audio1_io(address_map &map);
	void audio1_map(address_map &map);
	void audio2_io(address_map &map);
	void audio2_map(address_map &map);
	void main_map(address_map &map);
	void video_map(address_map &map);
	void wcup90_video_map(address_map &map);

	std::unique_ptr<bitmap_ind16> m_tile_bitmap;
	required_device<palette_device> m_palette;
	required_shared_ptr<uint8_t> m_p_videoram;
	required_shared_ptr<uint8_t> m_p_objectram;
	required_device<gfxdecode_device> m_gfxdecode;

	bool m_ack1;
	bool m_ack2;
	bool m_ackv;
	bool m_flip;
	bool m_intst;
	uint8_t m_irq_state;
	uint8_t m_row_data;
	uint8_t m_sound_data;
	uint8_t m_gfx_bank;
	uint8_t m_video_data;
	uint8_t m_video_status;

	required_device<m68000_device> m_maincpu;
	required_device<z80_device> m_audiocpu1;
	required_device<z80_device> m_audiocpu2;
	required_device<z80_device> m_videocpu;
	required_device<ls259_device> m_selectlatch;
	required_ioport m_io_dsw0;
	required_ioport m_io_dsw1;
	required_ioport m_io_x0;
	required_ioport m_io_x1;
};


void mrgame_state::main_map(address_map &map)
{
	map(0x000000, 0x00ffff).rom().region("roms", 0);
	map(0x020000, 0x02ffff).ram().share("nvram");
	map(0x030001, 0x030001).r(FUNC(mrgame_state::rsw_r)); //RSW ACK
	map(0x030003, 0x030003).w(FUNC(mrgame_state::sound_w)); //W SOUND
	map(0x030004, 0x030004).w(FUNC(mrgame_state::video_w)); //W VID
	map(0x030007, 0x030007).w(FUNC(mrgame_state::triple_w)); //W CS
	map(0x030008, 0x030009).nopw(); //W DATA - lamp/sol data
	map(0x03000b, 0x03000b).w(FUNC(mrgame_state::row_w)); //W ROW
	map(0x03000d, 0x03000d).r(FUNC(mrgame_state::col_r)); //R COL
	map(0x03000e, 0x03000f).nopw(); //EXT ADD - lamp/sol data
}

void mrgame_state::video_map(address_map &map)
{
	map(0x0000, 0x3fff).rom().region("video", 0);
	map(0x4000, 0x47ff).ram();
	map(0x4800, 0x4bff).mirror(0x0400).ram().share("videoram");
	map(0x5000, 0x50ff).mirror(0x0700).ram().share("objectram");
	map(0x6800, 0x6807).mirror(0x07f8).w(m_selectlatch, FUNC(ls259_device::write_d0));
	map(0x7000, 0x7000).mirror(0x07ff).nopr(); //AFR - watchdog reset
	map(0x8100, 0x8103).mirror(0x7efc).rw("ppi", FUNC(i8255_device::read), FUNC(i8255_device::write));
}

void mrgame_state::wcup90_video_map(address_map &map)
{
	map(0x0000, 0x7fff).rom().region("video", 0);
	map(0x8000, 0x87ff).ram();
	map(0x8800, 0x8bff).mirror(0x0400).ram().share("videoram");
	map(0x9000, 0x90ff).mirror(0x0700).ram().share("objectram");
	map(0xa800, 0xa807).mirror(0x07f8).w(m_selectlatch, FUNC(ls259_device::write_d0));
	map(0xb000, 0xb000).mirror(0x07ff).nopr(); //AFR - watchdog reset
	map(0xc000, 0xc003).mirror(0x3ffc).rw("ppi", FUNC(i8255_device::read), FUNC(i8255_device::write));
}

void mrgame_state::audio1_map(address_map &map)
{
	map(0x0000, 0x7fff).rom().region("audio1", 0);
	map(0xfc00, 0xffff).ram();
}

void mrgame_state::audio1_io(address_map &map)
{
	map.global_mask(3);
	map(0x0000, 0x0000).w("dacvol", FUNC(dac_byte_interface::data_w)); //DA1
	map(0x0001, 0x0001).r(FUNC(mrgame_state::sound_r)); //IN1
	map(0x0002, 0x0002).w(FUNC(mrgame_state::ack1_w)); //AKL1
	map(0x0003, 0x0003).nopw(); //SGS pass data to M114
}

void mrgame_state::audio2_map(address_map &map)
{
	map(0x0000, 0x7fff).rom().region("audio2", 0);
	map(0xfc00, 0xffff).ram();
}

void mrgame_state::audio2_io(address_map &map)
{
	map.global_mask(7);
	map(0x0000, 0x0000).w("ldac", FUNC(dac_byte_interface::data_w)); //DA2
	map(0x0001, 0x0001).r(FUNC(mrgame_state::sound_r)); //IN2
	map(0x0002, 0x0002).w(FUNC(mrgame_state::ack2_w)); //AKL2
	map(0x0003, 0x0003).rw("tms", FUNC(tms5220_device::status_r), FUNC(tms5220_device::data_w)); //Speech
	map(0x0004, 0x0004).w("rdac", FUNC(dac_byte_interface::data_w)); //DA3
}

static INPUT_PORTS_START( mrgame )
	PORT_START("DSW0")
	PORT_DIPNAME( 0x01, 0x00, "Ram Protect")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x0e, 0x0e, "Country")
	PORT_DIPSETTING(    0x00, "Italy 1")
	PORT_DIPSETTING(    0x02, "Italy")
	PORT_DIPSETTING(    0x04, "Great Britain")
	PORT_DIPSETTING(    0x06, "France")
	PORT_DIPSETTING(    0x08, "Germany")
	PORT_DIPSETTING(    0x0a, "Belgium")
	PORT_DIPSETTING(    0x0c, "Yugoslavia")
	PORT_DIPSETTING(    0x0e, "U.S.A.")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("R. Flipper") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("L. Flipper") PORT_CODE(KEYCODE_LSHIFT)

	// These dips are only documented for Motor Show
	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x00, "Test Game")
	PORT_DIPSETTING(    0x01, "Connected")
	PORT_DIPSETTING(    0x00, "Disconnected")
	PORT_DIPNAME( 0x02, 0x02, "Dragster")
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ))
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ))
	PORT_DIPNAME( 0x04, 0x04, "F.1.")
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ))
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ))
	PORT_DIPNAME( 0x08, 0x08, "Motocross")
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ))
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ))

	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Advance Test")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE3 ) PORT_NAME("Return Test")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE4 ) PORT_NAME("Factory Burn Test")
	PORT_BIT( 0xe9, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

uint8_t mrgame_state::rsw_r()
{
	return m_io_dsw0->read() | ((uint8_t)m_ack1 << 5) | ((uint8_t)m_ack2 << 4);
}

// this is like a keyboard, energise a row and read the column data
uint8_t mrgame_state::col_r()
{
	if (m_row_data == 0)
		return m_io_x0->read();
	else
	if (m_row_data == 1)
		return m_io_x1->read();
	else
	if (m_row_data == 7)
		return m_video_status;
	else

	return 0xff;
}

void mrgame_state::row_w(uint8_t data)
{
	m_row_data = data & 7;
}

uint8_t mrgame_state::sound_r()
{
	return m_sound_data;
}

void mrgame_state::sound_w(uint8_t data)
{
	m_sound_data = data;
	m_audiocpu1->set_input_line(INPUT_LINE_NMI, BIT(data, 7) ? CLEAR_LINE : ASSERT_LINE);
	m_audiocpu2->set_input_line(INPUT_LINE_NMI, BIT(data, 7) ? CLEAR_LINE : ASSERT_LINE);
}

// this produces 24 outputs from three driver chips to drive lamps & solenoids
void mrgame_state::triple_w(uint8_t data)
{
	if ((data & 0x18)==0)
		m_ackv = BIT(data, 7);
}

void mrgame_state::video_w(uint8_t data)
{
	m_video_data = data;
}

WRITE_LINE_MEMBER(mrgame_state::video_a11_w)
{
	m_gfx_bank = (m_gfx_bank & 6) | (state ? 1 : 0);
}

WRITE_LINE_MEMBER(mrgame_state::video_a12_w)
{
	m_gfx_bank = (m_gfx_bank & 5) | (state ? 2 : 0);
}

WRITE_LINE_MEMBER(mrgame_state::video_a13_w)
{
	m_gfx_bank = (m_gfx_bank & 3) | (state ? 4 : 0);
}

WRITE_LINE_MEMBER(mrgame_state::intst_w)
{
	m_intst = state;
	if (!state)
		m_videocpu->set_input_line(INPUT_LINE_IRQ0, CLEAR_LINE);
}

WRITE_LINE_MEMBER(mrgame_state::nmi_intst_w)
{
	m_intst = state;
	if (!state)
		m_videocpu->set_input_line(INPUT_LINE_NMI, CLEAR_LINE);
}

WRITE_LINE_MEMBER(mrgame_state::flip_w)
{
	m_flip = state;
}

void mrgame_state::ack1_w(uint8_t data)
{
	m_ack1 = BIT(data, 0);
}

void mrgame_state::ack2_w(uint8_t data)
{
	m_ack2 = BIT(data, 0);
}

uint8_t mrgame_state::porta_r()
{
	return m_video_data;
}

void mrgame_state::portb_w(uint8_t data)
{
	m_video_status = data;
	m_ackv = 0;
}

uint8_t mrgame_state::portc_r()
{
	return m_io_dsw1->read() | ((uint8_t)m_ackv << 4);
}

void mrgame_state::machine_start()
{
	m_tile_bitmap=std::make_unique<bitmap_ind16>(256,256);
}

void mrgame_state::machine_reset()
{
	m_sound_data = 0xff;
	m_irq_state = 0xff;
	m_video_data = 0;
	m_gfx_bank = 0;
	m_video_status = 0;
	m_ack1 = 0;
	m_ack2 = 0;
	m_ackv = 0;
	m_flip = 0;
	m_row_data = 0;
}

void mrgame_state::init_mrgame()
{
}

WRITE_LINE_MEMBER(mrgame_state::vblank_int_w)
{
	if (state && m_intst)
		m_videocpu->set_input_line(INPUT_LINE_IRQ0, ASSERT_LINE);
}

WRITE_LINE_MEMBER(mrgame_state::vblank_nmi_w)
{
	if (state && m_intst)
		m_videocpu->set_input_line(INPUT_LINE_NMI, ASSERT_LINE);
}

// This pulses the IRQ pins of both audio cpus. The schematic does not
//show which 4040 output is used, so we have guessed.
TIMER_DEVICE_CALLBACK_MEMBER(mrgame_state::irq_timer)
{
	m_irq_state++;
	// pulse_line of IRQ not allowed, so trying this instead
	if (m_irq_state == 254)
	{
		m_audiocpu1->set_input_line(INPUT_LINE_IRQ0, ASSERT_LINE);
		m_audiocpu2->set_input_line(INPUT_LINE_IRQ0, ASSERT_LINE);
	}
	else
	if (m_irq_state == 255)
	{
		m_audiocpu1->set_input_line(INPUT_LINE_IRQ0, CLEAR_LINE);
		m_audiocpu2->set_input_line(INPUT_LINE_IRQ0, CLEAR_LINE);
	}
}

static const gfx_layout charlayout =
{
	8, 8,
	4096,
	2,
	{ 0, 0x8000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56 },
	8*8
};

static const gfx_layout spritelayout =
{
	16, 16,
	1024,
	2,
	{ 0, 0x8000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 64, 65, 66, 67, 68, 69, 70, 71 },
	{ 0, 8, 16, 24, 32, 40, 48, 56, 128, 136, 144, 152, 160, 168, 176, 184 },
	32*8
};

static GFXDECODE_START(gfx_mrgame)
	GFXDECODE_ENTRY("chargen", 0, charlayout, 0, 16)
	GFXDECODE_ENTRY("chargen", 0, spritelayout, 0, 16)
GFXDECODE_END

void mrgame_state::mrgame_palette(palette_device &palette) const
{
	static constexpr int resistances[3] = { 1000, 470, 220 };
	uint8_t const *const color_prom = memregion("proms")->base();

	// compute the color output resistor weights
	double rweights[3], gweights[3], bweights[2];
	compute_resistor_weights(0, 255, -1.0,
			3, &resistances[0], rweights, 0, 0,
			3, &resistances[0], gweights, 0, 0,
			2, &resistances[1], bweights, 0, 0);

	// create a lookup table for the palette
	for (uint8_t i = 0; i < 32; i++)
	{
		uint8_t bit0, bit1, bit2;

		// red component
		bit0 = BIT(color_prom[i], 0);
		bit1 = BIT(color_prom[i], 1);
		bit2 = BIT(color_prom[i], 2);
		uint8_t const r = combine_weights(rweights, bit0, bit1, bit2);

		// green component
		bit0 = BIT(color_prom[i], 3);
		bit1 = BIT(color_prom[i], 4);
		bit2 = BIT(color_prom[i], 5);
		uint8_t const g = combine_weights(gweights, bit0, bit1, bit2);

		// blue component
		bit0 = BIT(color_prom[i], 6);
		bit1 = BIT(color_prom[i], 7);
		uint8_t const b = combine_weights(bweights, bit0, bit1);

		palette.set_pen_color(i, rgb_t(r, g, b));
		palette.set_pen_color(i + 32, rgb_t(r, g, b));
	}
}

uint32_t mrgame_state::screen_update_mrgame(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	return 0;
}

void mrgame_state::mrgame(machine_config &config)
{
	/* basic machine hardware */
	M68000(config, m_maincpu, 6_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &mrgame_state::main_map);
	m_maincpu->set_periodic_int(FUNC(mrgame_state::irq1_line_hold), attotime::from_hz(183));

	Z80(config, m_videocpu, 18.432_MHz_XTAL / 6);
	m_videocpu->set_addrmap(AS_PROGRAM, &mrgame_state::video_map);

	Z80(config, m_audiocpu1, 4_MHz_XTAL);
	m_audiocpu1->set_addrmap(AS_PROGRAM, &mrgame_state::audio1_map);
	m_audiocpu1->set_addrmap(AS_IO, &mrgame_state::audio1_io);

	Z80(config, m_audiocpu2, 4_MHz_XTAL);
	m_audiocpu2->set_addrmap(AS_PROGRAM, &mrgame_state::audio2_map);
	m_audiocpu2->set_addrmap(AS_IO, &mrgame_state::audio2_io);

	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0); // 5564 (x2) + battery

	LS259(config, m_selectlatch); // 5B
	m_selectlatch->q_out_cb<0>().set(FUNC(mrgame_state::video_a11_w));
	m_selectlatch->q_out_cb<1>().set(FUNC(mrgame_state::nmi_intst_w));
	m_selectlatch->q_out_cb<3>().set(FUNC(mrgame_state::video_a12_w));
	m_selectlatch->q_out_cb<4>().set(FUNC(mrgame_state::video_a13_w));
	m_selectlatch->q_out_cb<6>().set(FUNC(mrgame_state::flip_w));

	//watchdog_timer_device &watchdog(WATCHDOG_TIMER(config, "watchdog")); // LS393 at 5D (video board) driven by VBLANK
	//watchdog.set_vblank_count("screen", 8);

	/* video hardware */
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_raw(18.432_MHz_XTAL / 3, 384, 0, 256, 264, 8, 248); // If you align with X on test screen some info is chopped off
	screen.set_screen_update(FUNC(mrgame_state::screen_update_mrgame));
	screen.set_palette(m_palette);
	screen.screen_vblank().set(FUNC(mrgame_state::vblank_nmi_w));

	PALETTE(config, m_palette, FUNC(mrgame_state::mrgame_palette), 64);

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_mrgame);

	/* Sound */
	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();
	DAC_8BIT_R2R(config, "ldac", 0).add_route(ALL_OUTPUTS, "lspeaker", 0.25); // unknown DAC
	DAC_8BIT_R2R(config, "rdac", 0).add_route(ALL_OUTPUTS, "rspeaker", 0.25); // unknown DAC

	dac_8bit_r2r_device &dacvol(DAC_8BIT_R2R(config, "dacvol", 0));
	dacvol.set_output_range(0, 1); // unknown DAC
	dacvol.add_route(0, "ldac", 1.0, DAC_INPUT_RANGE_HI);
	dacvol.add_route(0, "ldac", -1.0, DAC_INPUT_RANGE_LO);
	dacvol.add_route(0, "rdac", 1.0, DAC_INPUT_RANGE_HI);
	dacvol.add_route(0, "rdac", -1.0, DAC_INPUT_RANGE_LO);

	tms5220_device &tms(TMS5220(config, "tms", 672000)); // uses a RC combination. 672k copied from jedi.h
	tms.ready_cb().set_inputline("audiocpu2", Z80_INPUT_LINE_BOGUSWAIT);
	tms.add_route(ALL_OUTPUTS, "lspeaker", 1.0);
	tms.add_route(ALL_OUTPUTS, "rspeaker", 1.0);

	/* Devices */
	TIMER(config, "irq_timer").configure_periodic(FUNC(mrgame_state::irq_timer), attotime::from_hz(16000)); //ugh

	i8255_device &ppi(I8255A(config, "ppi"));
	ppi.in_pa_callback().set(FUNC(mrgame_state::porta_r));
	ppi.out_pb_callback().set(FUNC(mrgame_state::portb_w));
	ppi.in_pc_callback().set(FUNC(mrgame_state::portc_r));
}

void mrgame_state::wcup90(machine_config &config)
{
	mrgame(config);

	m_videocpu->set_addrmap(AS_PROGRAM, &mrgame_state::wcup90_video_map);

	m_selectlatch->q_out_cb<1>().set(FUNC(mrgame_state::intst_w)); // U48

	subdevice<screen_device>("screen")->screen_vblank().set(FUNC(mrgame_state::vblank_int_w));
}

/*-------------------------------------------------------------------
/ Dakar (06/1988)
/-------------------------------------------------------------------*/
ROM_START(dakar)
	ROM_REGION16_BE(0x10000, "roms", 0)
	ROM_LOAD16_BYTE("cpu_ic13.rom", 0x000000, 0x8000, CRC(83183929) SHA1(977ac10a1e78c759eb0550794f2639fe0e2d1507))
	ROM_LOAD16_BYTE("cpu_ic14.rom", 0x000001, 0x8000, CRC(2010d28d) SHA1(d262dabd9298566df43df298cf71c974bee1434a))

	ROM_REGION(0x8000, "video", 0)
	ROM_LOAD("vid_ic14.rom", 0x0000, 0x8000, CRC(88a9ca81) SHA1(9660d416b2b8f1937cda7bca51bd287641c7730c))

	ROM_REGION(0x10000, "chargen", 0)
	ROM_LOAD("vid_ic55.rom", 0x0000, 0x8000, CRC(3c68b448) SHA1(f416f00d2de0c71c021fec0e9702ba79b761d5e7))
	ROM_LOAD("vid_ic56.rom", 0x8000, 0x8000, CRC(0aac43e9) SHA1(28edfeddb2d54e40425488bad37e3819e4488b0b))

	ROM_REGION(0x0020, "proms", 0)
	ROM_LOAD("vid_ic66.rom", 0x0000, 0x0020, CRC(c8269b27) SHA1(daa83bfdb1e255b846bbade7f200abeaa9399c06))

	ROM_REGION(0x10000, "audio1", 0)
	ROM_LOAD("snd_ic06.rom", 0x0000, 0x8000, CRC(29e9417e) SHA1(24f465993da7c93d385ec453497f2af4d8abb6f4))
	ROM_LOAD("snd_ic07.rom", 0x8000, 0x8000, CRC(71ab15fe) SHA1(245842bb41410ea481539700f79c7ef94f8f8924))

	ROM_REGION(0x4000, "m114", 0)
	ROM_LOAD("snd_ic22.rom", 0x0000, 0x4000, CRC(e6c1098e) SHA1(06bf8917a27d5e46e4aab93e1f212918418e3a82))

	ROM_REGION(0x10000, "audio2", 0)
	ROM_LOAD("snd_ic35.rom", 0x0000, 0x8000, CRC(7b2394d1) SHA1(f588f5105d75b54dd65bb6448a2d7774fb8477ec))
	ROM_LOAD("snd_ic36.rom", 0x8000, 0x8000, CRC(4039ea65) SHA1(390fce94d1e48b395157d8d9afaa485114c58d52))
ROM_END

/*-------------------------------------------------------------------
/ Motor Show (1989)
/-------------------------------------------------------------------*/
ROM_START(motrshow)
	ROM_REGION16_BE(0x10000, "roms", 0)
	ROM_LOAD16_BYTE("cpu_ic13.rom", 0x000000, 0x8000, CRC(e862ca71) SHA1(b02e5f39f9427d58b70b7999a5ff6075beff05ae))
	ROM_LOAD16_BYTE("cpu_ic14.rom", 0x000001, 0x8000, CRC(c898ae25) SHA1(f0e1369284a1e0f394f1d40281fd46252016602e))

	ROM_REGION(0x8000, "video", 0)
	ROM_LOAD("vid_ic14.rom", 0x0000, 0x8000, CRC(1d4568e2) SHA1(bfc2bb59708ce3a09f9a1b3460ed8d5269840c97))

	ROM_REGION(0x10000, "chargen", 0)
	ROM_LOAD("vid_ic55.rom", 0x0000, 0x8000, CRC(c27a4ded) SHA1(9c2c9b17f1e71afb74bdfbdcbabb99ef935d32db))
	ROM_LOAD("vid_ic56.rom", 0x8000, 0x8000, CRC(1664ec8d) SHA1(e7b15acdac7dfc51b668e908ca95f02a2b569737))

	ROM_REGION(0x0020, "proms", 0)
	ROM_LOAD("vid_ic66.rom", 0x0000, 0x0020, CRC(5b585252) SHA1(b88e56ebdce2c3a4b170aff4b05018e7c21a79b8))

	ROM_REGION(0x10000, "audio1", 0)
	ROM_LOAD("snd_ic06.rom", 0x0000, 0x8000, CRC(fba5a8f1) SHA1(ddf989abebe05c569c9ecdd498bd8ea409df88ac))

	ROM_REGION(0x4000, "m114", 0)
	ROM_LOAD("snd_ic22.rom", 0x0000, 0x4000, CRC(e6c1098e) SHA1(06bf8917a27d5e46e4aab93e1f212918418e3a82))

	ROM_REGION(0x10000, "audio2", 0)
	ROM_LOAD("snd_ic35.rom", 0x0000, 0x8000, CRC(9dec153d) SHA1(8a0140257316aa19c0401456839e11b6896609b1))
	ROM_LOAD("snd_ic36.rom", 0x8000, 0x8000, CRC(4f42be6e) SHA1(684e988f413cd21c785ad5d60ef5eaddddaf72ab))
ROM_END

ROM_START(motrshowa)
	ROM_REGION16_BE(0x10000, "roms", 0)
	ROM_LOAD16_BYTE("cpuic13a.rom", 0x000000, 0x8000, CRC(2dbdd9d4) SHA1(b404814a4e83ead6da3c57818ae97f23d380f9da))
	ROM_LOAD16_BYTE("cpuic14b.rom", 0x000001, 0x8000, CRC(0bd98fec) SHA1(b90a7e997db59740398003ba94a69118b1ee70af))

	ROM_REGION(0x8000, "video", 0)
	ROM_LOAD("vid_ic14.rom", 0x0000, 0x8000, CRC(1d4568e2) SHA1(bfc2bb59708ce3a09f9a1b3460ed8d5269840c97))

	ROM_REGION(0x10000, "chargen", 0)
	ROM_LOAD("vid_ic55.rom", 0x0000, 0x8000, CRC(c27a4ded) SHA1(9c2c9b17f1e71afb74bdfbdcbabb99ef935d32db))
	ROM_LOAD("vid_ic56.rom", 0x8000, 0x8000, CRC(1664ec8d) SHA1(e7b15acdac7dfc51b668e908ca95f02a2b569737))

	ROM_REGION(0x0020, "proms", 0)
	ROM_LOAD("vid_ic66.rom", 0x0000, 0x0020, CRC(5b585252) SHA1(b88e56ebdce2c3a4b170aff4b05018e7c21a79b8))

	ROM_REGION(0x10000, "audio1", 0)
	ROM_LOAD("snd_ic06.rom", 0x0000, 0x8000, CRC(fba5a8f1) SHA1(ddf989abebe05c569c9ecdd498bd8ea409df88ac))

	ROM_REGION(0x4000, "m114", 0)
	ROM_LOAD("snd_ic22.rom", 0x0000, 0x4000, CRC(e6c1098e) SHA1(06bf8917a27d5e46e4aab93e1f212918418e3a82))

	ROM_REGION(0x10000, "audio2", 0)
	ROM_LOAD("snd_ic35.rom", 0x0000, 0x8000, CRC(9dec153d) SHA1(8a0140257316aa19c0401456839e11b6896609b1))
	ROM_LOAD("snd_ic36.rom", 0x8000, 0x8000, CRC(4f42be6e) SHA1(684e988f413cd21c785ad5d60ef5eaddddaf72ab))
ROM_END

/*-------------------------------------------------------------------
/ Mac Attack (1990)
/-------------------------------------------------------------------*/
ROM_START(macattck)
	ROM_REGION16_BE(0x10000, "roms", 0)
	ROM_LOAD16_BYTE("cpu_ic13.rom", 0x0000, 0x8000, CRC(35cabad1) SHA1(01279df881b0d7d6586c1b8570b12bdc1fb9ff21) )
	ROM_LOAD16_BYTE("cpu_ic14.rom", 0x0001, 0x8000, CRC(6a4d7b89) SHA1(090e1a6c069cb6e5efd26a0260df613375f0b063) )

	ROM_REGION(0x8000, "video", 0)
	ROM_LOAD("vid_ic91.rom", 0x0000, 0x8000, CRC(42d2ba01) SHA1(c13d38c2798575760461912cef65dde57dfd938c) )

	ROM_REGION(0x28000, "chargen", 0)
	ROM_LOAD("vid_ic14.rom", 0x00000, 0x8000, CRC(f6e047fb) SHA1(6be712dda60257b9e7014315c8fee19812622bf6) )
	ROM_LOAD("vid_ic15.rom", 0x08000, 0x8000, CRC(405a8f54) SHA1(4d58915763db3c3be2bfc166be1a12285ff2c38b) )
	ROM_LOAD("vid_ic16.rom", 0x10000, 0x8000, CRC(063ea783) SHA1(385dbfcc8ecd3a784f9a8752d00e060b48d70d6a) )
	ROM_LOAD("vid_ic17.rom", 0x18000, 0x8000, CRC(7494e44e) SHA1(c7c062508e81b9fd818f36f80d4a6da02c3bda40) )
	ROM_LOAD("vid_ic18.rom", 0x20000, 0x8000, CRC(83ef25f8) SHA1(bab482badb8646b099dbb197ca9af3a126b274e3) )

	ROM_REGION(0x0020, "proms", 0)
	ROM_LOAD("vid_ic61.rom", 0x0000, 0x0020, CRC(538c72ae) SHA1(f704492568257fcc4a4f1189207c6fb6526eb81c) BAD_DUMP) // from wcup90, assumed to be the same

	ROM_REGION(0x10000, "audio1", 0)
	ROM_LOAD("snd_ic06.rom", 0x0000, 0x8000, CRC(4ab94d16) SHA1(4c3755489f699c751d664f420b9852ef16bb3aa6) )

	ROM_REGION(0x4000, "m114", 0)
	ROM_LOAD("snd_ic22.rom", 0x0000, 0x4000, CRC(9d3546c5) SHA1(cc6e91288692b927f7d046e192b1fd128c126d0d) )

	ROM_REGION(0x10000, "audio2", 0)
	ROM_LOAD("snd_ic35.rom", 0x0000, 0x8000, CRC(52e9811c) SHA1(52223cf14a185b4dab14143d797000baf6d618cc) )
	ROM_LOAD("snd_ic36.rom", 0x8000, 0x8000, CRC(2e6b5822) SHA1(9e390e4b71cc103ec3d781575df484a3e4217b3b) )
ROM_END

/*-------------------------------------------------------------------
/ World Cup 90 (1990)
/-------------------------------------------------------------------*/
ROM_START(wcup90)
	ROM_REGION16_BE(0x10000, "roms", 0)
	ROM_LOAD16_BYTE("cpu_ic13.rom", 0x000000, 0x8000, CRC(0e2edfb0) SHA1(862fb1f6509fb1f560d0b2bb8a5764f64b259f04))
	ROM_LOAD16_BYTE("cpu_ic14.rom", 0x000001, 0x8000, CRC(fdd03165) SHA1(6dc6e68197218f8808436098c26cd04fc3215b1c))

	ROM_REGION(0x8000, "video", 0)
	ROM_LOAD("vid_ic91.rom", 0x0000, 0x8000, CRC(3287ad20) SHA1(d5a453efc7292670073f157dca04897be857b8ed))

	ROM_REGION(0x30000, "chargen", 0)
	ROM_LOAD("vid_ic14.rom", 0x00000, 0x8000, CRC(a101d562) SHA1(ad9ad3968f13169572ec60e22e84acf43382b51e))
	ROM_LOAD("vid_ic15.rom", 0x08000, 0x8000, CRC(40791e7a) SHA1(788760b8527df48d1825be88099491b6e94f0a19))
	ROM_LOAD("vid_ic16.rom", 0x10000, 0x8000, CRC(a7214157) SHA1(a4660180e8491a37028fec8533cf13daf839a7c4))
	ROM_LOAD("vid_ic17.rom", 0x18000, 0x8000, CRC(caf4fb04) SHA1(81784a4dc7c671090cf39cafa7d34a6b34523168))
	ROM_LOAD("vid_ic18.rom", 0x20000, 0x8000, CRC(83ad2a10) SHA1(37664e5872e6322ee6bb61ec9385876626598152))

	ROM_REGION(0x0020, "proms", 0)
	ROM_LOAD("vid_ic61.rom", 0x0000, 0x0020, CRC(538c72ae) SHA1(f704492568257fcc4a4f1189207c6fb6526eb81c))

	ROM_REGION(0x10000, "audio1", 0)
	ROM_LOAD("snd_ic06.rom", 0x0000, 0x8000, CRC(19a66331) SHA1(fbd71bc378b5a04247fd1754529c66b086eb33d8))

	ROM_REGION(0x4000, "user1", 0)
	ROM_LOAD("snd_ic21.rom", 0x0000, 0x4000, CRC(e6c1098e) SHA1(06bf8917a27d5e46e4aab93e1f212918418e3a82))

	ROM_REGION(0x30000, "user2", 0)
	ROM_LOAD("snd_ic45.rom", 0x00000, 0x10000, CRC(265aa979) SHA1(9ca10c41526a2d227c21f246273ca14bec7f1bc7))
	ROM_LOAD("snd_ic46.rom", 0x10000, 0x10000, CRC(7edb321e) SHA1(b242e94c24e996d2de803d339aa9bf6e93586a4c))

	ROM_REGION(0x10000, "audio2", 0)
	ROM_LOAD("snd_ic44.rom", 0x00000, 0x8000, CRC(00946570) SHA1(83e7dd89844679571ab2a803295c8ca8941a4ac7))
ROM_END

} // anonymous namespace


GAME(1988,  dakar,     0,         mrgame,  mrgame, mrgame_state, init_mrgame, ROT0, "Mr Game", "Dakar",              MACHINE_MECHANICAL | MACHINE_NOT_WORKING | MACHINE_IMPERFECT_SOUND )
GAME(1989,  motrshow,  0,         mrgame,  mrgame, mrgame_state, init_mrgame, ROT0, "Mr Game", "Motor Show (set 1)", MACHINE_MECHANICAL | MACHINE_NOT_WORKING | MACHINE_IMPERFECT_SOUND )
GAME(1989,  motrshowa, motrshow,  mrgame,  mrgame, mrgame_state, init_mrgame, ROT0, "Mr Game", "Motor Show (set 2)", MACHINE_MECHANICAL | MACHINE_NOT_WORKING | MACHINE_IMPERFECT_SOUND )
GAME(1990,  macattck,  0,         wcup90,  mrgame, mrgame_state, init_mrgame, ROT0, "Mr Game", "Mac Attack",         MACHINE_IS_SKELETON_MECHANICAL)
GAME(1990,  wcup90,    0,         wcup90,  mrgame, mrgame_state, init_mrgame, ROT0, "Mr Game", "World Cup 90",       MACHINE_MECHANICAL | MACHINE_NOT_WORKING | MACHINE_IMPERFECT_SOUND | MACHINE_IMPERFECT_GRAPHICS )
