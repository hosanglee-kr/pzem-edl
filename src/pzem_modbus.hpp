/*
PZEM EDL - PZEM Event Driven Library

This code implements communication and data exchange with PZEM004T V3.0 module using MODBUS proto
and provides an API for energy metrics monitoring and data processing.

This file is part of the 'PZEM event-driven library' project.

Copyright (C) Emil Muratov, 2021
GitHub: https://github.com/vortigont/pzem-edl
*/

#pragma once
#include <Arduino.h>
#include "uartq.hpp"

// Read-Only 16-bit registers
#define REG_VOLTAGE             0x0000  // 1LSB correspond to 0.1 V
#define REG_CURRENT_L           0x0001  // 1LSB correspond to 0.001 A
#define REG_CURRENT_H           0X0002
#define REG_POWER_L             0x0003  // 1LSB correspond to 0.1 W
#define REG_POWER_H             0x0004
#define REG_ENERGY_L            0x0005  // 1LSB correspond to 1 W*h
#define REG_ENERGY_H            0x0006
#define REG_FREQUENCY           0x0007  // 1LSB correspond to 0.1 Hz
#define REG_PF                  0x0008  // 1LSB correspond to 0.01
#define REG_ALARM               0x0009  // 0xFFFF is alarm / 0x0000 is not alarm
#define REG_METER_DATA_START    REG_VOLTAGE
#define REG_METER_DATA_LEN      0x0A
#define REG_METER_RESP_LEN      0x14

// RW 16-bit registers
#define WREG_ALARM_THR          0x0001  // Alarm threshold, 1LSB correspond to 1W
#define WREG_ADDR               0x0002  // MODBUS Slave address register   (The range is 0x0001~0x00F7)
#define WREG_BEGIN              WREG_ALARM_THR
#define WREG_LEN                2


// Commands
#define CMD_RHR                 0x03    // Read Holding Register    (Read RW regs)
#define CMD_RIR                 0X04    // Read Input Register      (Read RO regs)
#define CMD_WSR                 0x06    // Write Single Register
#define CMD_CAL                 0x41    // Calibration
#define CMD_RST_ENRG            0x42    // Reset energy
#define CMD_RERR                0x84    // Read  Command error
#define CMD_WERR                0x86    // Write Command error
#define CMD_CALERR              0xC1    // Calibration Command error
#define CMD_RSTERR              0xC2    // Reset Energy Command error

// Slave addressing
#define ADDR_BCAST              0x00    // broadcast address    (slaves are not supposed to answer here)
#define ADDR_MIN                0x01    // lowest slave address
#define ADDR_MAX                0xF7    // highest slave address
#define ADDR_ANY                0xF8    // default catch-all address

// ERR Codes
#define ERR_FUNC                0x01    // Illegal function
#define ERR_ADDR                0x02    // Illegal address
#define ERR_DATA                0x03    // Illegal data
#define ERR_SLAVE               0x04    // Slave error

// Factory calibration
#define CAL_ADDR                ADDR_ANY    // Calibration address
#define CAL_PWD                 0x3721      // Calibration password

// Power Alarm
#define ALARM_PRESENT           0xffff
#define ALARM_ABSENT            0x0000

#define GENERIC_MSG_SIZE        8
#define ENERGY_RST_MSG_SIZE     4
#define REPORT_ADDR_MSG_SIZE    5 

#define PZEM_REFRESH_PERIOD     1000    // PZEM updates it's internal register data every ~1 sec


// defines for PZEM003 device
// RO regs
#define PZ003_RIR_VOLTAGE       0x00    // 1LSB correspond to 0.01 V
#define PZ003_RIR_CURRENT       0x01    // 1LSB correspond to 0.01 A
#define PZ003_RIR_POWER_L       0x02    // 1LSB correspond to 0.1 W
#define PZ003_RIR_POWER_H       0x03
#define PZ003_RIR_ENERGY_L      0x04    // 1LSB correspond to 1 Wh
#define PZ003_RIR_ENERGY_H      0x05
#define PZ003_RIR_ALARM_H       0x06    // 0xFFFF is alarm,0x0000 is not alarm
#define PZ003_RIR_ALARM_L       0x07    // 0xFFFF is alarm,0x0000 is not alarm
#define PZ003_RIR_DATA_BEGIN    PZ003_RIR_VOLTAGE    // 8 regs total
#define PZ003_RIR_DATA_LEN      0x08    // 8 regs total
#define PZ003_RIR_RESP_LEN      0x10    // resp len is 16 bytes

// RW regs
#define PZ003_RHR_ALARM_H       0x00    // 1LSB correspond to 0.01 V
#define PZ003_RHR_ALARM_L       0x01    // 1LSB correspond to 0.01 A
#define PZ003_RHR_ADDR          0x02    // 1LSB correspond to 0.1 W
#define PZ003_RHR_CURRENT_RANGE 0x03    // 0x0000：100A 0x0001：50A 0x0002：200A 0x0003：300A
#define PZ003_RHR_BEGIN         PZ003_RHR_ALARM_H
#define PZ003_RHR_CNT           4       // number of RHR regs

// ESP32 is little endian here

namespace pzmbus {

enum class pzemcmd_t:uint8_t {
    RHR = CMD_RHR,
    RIR = CMD_RIR,
    WSR = CMD_WSR,
    calibrate = CMD_CAL,
    reset_energy = CMD_RST_ENRG,
    read_err = CMD_RERR,
    write_err = CMD_WERR,
    calibrate_err = CMD_CALERR,
    reset_err = CMD_RSTERR
};

// Some of the possible Error states
enum class pzem_err_t:uint8_t {
    err_ok = 0,
    err_func = ERR_FUNC,
    err_addr = ERR_ADDR,
    err_data = ERR_DATA,
    err_slave = ERR_SLAVE,
    err_parse                   // error parsing reply
};

/**
 * @brief Create a msg object with PZEM command wrapped into proper MODBUS message
 * this is a genereic command template
 * 
 * @param cmd - PZEM command
 * @param reg_addr - register address
 * @param value - command value
 * @param slave_addr - slave device modbus address
 * @param w4r - 'wait-4-reply' expexted flag
 * @return TX_msg* 
 */
TX_msg* create_msg(uint8_t cmd, uint16_t reg_addr, uint16_t value, uint8_t slave_addr = ADDR_ANY, bool w4r = true);

/**
 * @brief  message request to change slave device modbus address
 * 
 * @param addr - new modbus address
 * @param current_addr - current modbus address
 * @return TX_msg* 
 */
TX_msg* cmd_set_modbus_addr(uint8_t addr, const uint8_t current_addr = ADDR_ANY);

/**
 * @brief create MSG - reset PZEM's Energy counter to zero
 * 
 * @param addr - device address
 * @return TX_msg* pointer to the message struct
 */
TX_msg* cmd_energy_reset(const uint8_t addr = ADDR_ANY);

/**
 * @brief dump content of received packet to the stdout
 * 
 * @param m 
 */
void rx_msg_debug(const RX_msg *m);

/**
 * @brief dump content of transmitted packet to the stdout
 * 
 * @param m 
 */
void tx_msg_debug(const TX_msg *m);
}   // end of 'namespace pzmbus'


namespace pz004 {

// Enumeration of available energy metrics
enum class meter_t:uint8_t { vol, cur, pwr, enrg, frq, pf, alrm };

// Enumeration of available MODBUS commands


/**
 * @brief struct with energy metrics data
 * contains raw-mapped byte values
 * 
 * this struct is nicely 32-bit aligned :)
 */
struct metrics {
    uint16_t voltage=0;
    uint32_t current=0;
    uint32_t power=0;
    uint32_t energy=0;
    uint16_t freq=0;
    uint16_t pf=0;
    uint16_t alarm=0;

    float asFloat(meter_t m) const {
        switch (m)
        {
        case meter_t::vol :
            return voltage / 10.0;
            break;
        case meter_t::cur :
            return current / 1000.0;
            break;
        case meter_t::pwr :
            return power / 10.0;
            break;
        case meter_t::enrg :
            return static_cast< float >(energy);
            break;
        case meter_t::frq :
            return freq / 10.0;
            break;
        case meter_t::pf :
            return pf / 100.0;
            break;
        case meter_t::alrm :
            return alarm ? 1.0 : 0.0;
            break;
        default:
            return NAN;
        }
    }

    bool parse_rx_msg(const RX_msg *m){
        if (static_cast<pzmbus::pzemcmd_t>(m->cmd) != pzmbus::pzemcmd_t::RIR || m->rawdata[2] != REG_METER_RESP_LEN)
            return false;

        uint8_t const *value = &m->rawdata[3];

        voltage = __builtin_bswap16(*(uint16_t*)&value[REG_VOLTAGE*2]);
        current = __builtin_bswap16(*(uint16_t*)&value[REG_CURRENT_L*2]) | __builtin_bswap16(*(uint16_t*)&value[REG_CURRENT_H*2]  << 16);
        power   = __builtin_bswap16(*(uint16_t*)&value[REG_POWER_L*2])   | __builtin_bswap16(*(uint16_t*)&value[REG_POWER_H*2]    << 16);
        energy  = __builtin_bswap16(*(uint16_t*)&value[REG_ENERGY_L*2])  | __builtin_bswap16(*(uint16_t*)&value[REG_ENERGY_H*2]   << 16);
        freq    = __builtin_bswap16(*(uint16_t*)&value[REG_FREQUENCY*2]);
        pf      = __builtin_bswap16(*(uint16_t*)&value[REG_PF*2]);
        alarm   = __builtin_bswap16(*(uint16_t*)&value[REG_ALARM*2]);
        return true;
    }
};

/**
 * @brief a structure that reflects PZEM state/data values
 * 
 */
struct pzem_state {
    uint8_t addr = ADDR_ANY;
    metrics data;
    uint16_t alrm_thrsh=0;
    bool alarm=false;
    pzmbus::pzem_err_t err;
    int64_t poll_us=0;     // last poll request sent time, microseconds since boot
    int64_t update_us=0;   // last succes update time, us since boot

    /**
     * @brief return age time since last update in ms
     * 
     * @return int64_t age time in ms
     */
    int64_t dataAge() const { return (esp_timer_get_time() - update_us)/1000; }

    /**
     * @brief update poll_us to current value
     * should be called on each request set to PZEM
     * 
     */
    void reset_poll_us(){ poll_us=esp_timer_get_time(); }

    /**
     * @brief data considered stale if last update time is more than PZEM_REFRESH_PERIOD
     * 
     * @return true if stale
     * @return false if data is fresh and valid
     */
    bool dataStale() const {return (esp_timer_get_time() - update_us > PZEM_REFRESH_PERIOD );}

    /**
     * @brief try to parse PZEM reply packet and update structure state
     * 
     * @param m RX message struct
     * @param skiponbad try to parse even packet has wrong MODBUS add or has bad CRC
     * @return true on success
     * @return false on error
     */
    bool parse_rx_mgs(const RX_msg *m, bool skiponbad=true){
        if (!m->valid && skiponbad)          // check if message is valid before parsing it further
            return false;

        if (m->addr != addr && skiponbad)    // this is not "my" packet
            return false;

        switch (static_cast<pzmbus::pzemcmd_t>(m->cmd)){
            case pzmbus::pzemcmd_t::RIR : {
                if(data.parse_rx_msg(m))  // try to parse it as a full metrics packet
                    break;
                else {
                    err = pzmbus::pzem_err_t::err_parse;
                    return false;
                }
            }
            case pzmbus::pzemcmd_t::RHR : {
                if (m->rawdata[2] == WREG_LEN * 2){ // we got full len RHR data
                    alrm_thrsh = __builtin_bswap16(*(uint16_t*)&m->rawdata[3]);
                    addr = m->rawdata[6];
                }
                // unknown regs
                break;
            }
            case pzmbus::pzemcmd_t::WSR : {
                // 4th byte is reg ADDR_L
                if (m->rawdata[3] == WREG_ADDR){
                    addr = m->rawdata[5];            // addr is only one byte
                    break;
                } else if(m->rawdata[3] == WREG_ALARM_THR){
                    alrm_thrsh = __builtin_bswap16(*(uint16_t*)&m->rawdata[4]);
                }
                break;
            }
            case pzmbus::pzemcmd_t::reset_energy :
                data.energy=0;                      // nothing to do, except reset conter
                break;
            case pzmbus::pzemcmd_t::read_err :
            case pzmbus::pzemcmd_t::write_err :
            case pzmbus::pzemcmd_t::reset_err :
            case pzmbus::pzemcmd_t::calibrate_err :
                // стоит ли здесь инвалидировать метрики???
                err = (pzmbus::pzem_err_t)m->rawdata[2];
                return true;
            default:
                break;
        }

        err = pzmbus::pzem_err_t::err_ok;
        update_us = esp_timer_get_time();
        return true;
    }

};

/**
 * @brief message request for all energy metrics
 * 
 * @param addr - slave device modbus address
 * @return TX_msg* 
 */
TX_msg* cmd_get_metrics(uint8_t addr = ADDR_ANY);

/**
 * @brief message request to get RHR values
 * there two regs - 'slave modbus addr' and 'alarm threshold', this will read both.
 * It is not possible to distinguish reply packets for one reg from another if reading only one reg
 * so we will read both and parse data output for required one
 * 
 * @param addr 
 * @return TX_msg* 
 */
TX_msg* cmd_get_opts(const uint8_t addr = ADDR_ANY);

TX_msg* cmd_set_modbus_addr(uint8_t new_addr, const uint8_t current_addr = ADDR_ANY);

/**
 * @brief message request to report current slave device modbus address
 * 
 * @param addr 
 * @return TX_msg* 
 */
TX_msg* cmd_get_modbus_addr(const uint8_t addr = ADDR_ANY);

/**
 * @brief message request to set new Power Alarm threshold value
 * 
 * @param w - watts threshold value
 * @param addr 
 * @return TX_msg* 
 */
TX_msg* cmd_set_alarm_thr(uint16_t w, const uint8_t addr = ADDR_ANY);

/**
 * @brief message request to get current Power Alarm threshold value
 * 
 * @param addr 
 * @return TX_msg* 
 */
TX_msg* cmd_get_alarm_thr(const uint8_t addr = ADDR_ANY);

TX_msg* cmd_energy_reset(const uint8_t addr = ADDR_ANY);

/**
 * @brief dump content of received packet to the stdout
 * 
 * @param m 
 */
void rx_msg_debug(const RX_msg *m);

/**
 * @brief dump content of transmitted packet to the stdout
 * 
 * @param m 
 */
void tx_msg_debug(const TX_msg *m);

/**
 * @brief pretty print the content of RX packet data
 * 
 * @param m PZEM RX packet structure
 */
void rx_msg_prettyp(const RX_msg *m);
}



// Implementation for PZEM003
namespace pz003 {

// Enumeration of available energy metrics
enum class meter_t:uint8_t { vol, cur, pwr, enrg, alrmh, alrml };

// Enumeration of available MODBUS commands

/**
 * @brief struct with energy metrics data
 * contains raw-mapped byte values
 * 
 * this struct is nicely 32-bit aligned :)
 */
struct metrics {
    uint16_t voltage=0;
    uint16_t current=0;
    uint32_t power=0;
    uint32_t energy=0;
    uint16_t alarmh=0;
    uint16_t alarml=0;

    float asFloat(meter_t m) const {
        switch (m){
        case meter_t::vol :
            return voltage / 100.0;
            break;
        case meter_t::cur :
            return current / 100.0;
            break;
        case meter_t::pwr :
            return power / 10.0;
            break;
        case meter_t::enrg :
            return static_cast< float >(energy);
            break;
        case meter_t::alrmh :
            return alarmh ? 1.0 : 0.0;
            break;
        case meter_t::alrml :
            return alarml ? 1.0 : 0.0;
            break;
        default:
            return NAN;
        }
    }

    bool parse_rx_msg(const RX_msg *m){
        if (static_cast<pzmbus::pzemcmd_t>(m->cmd) != pzmbus::pzemcmd_t::RIR || m->rawdata[2] != PZ003_RIR_RESP_LEN)
            return false;

        uint8_t const *value = &m->rawdata[3];

        voltage = __builtin_bswap16(*(uint16_t*)&value[PZ003_RIR_VOLTAGE*2]);
        current = __builtin_bswap16(*(uint16_t*)&value[PZ003_RIR_CURRENT*2]);
        power   = __builtin_bswap16(*(uint16_t*)&value[PZ003_RIR_POWER_L*2])   | __builtin_bswap16(*(uint16_t*)&value[PZ003_RIR_POWER_H*2]    << 16);
        energy  = __builtin_bswap16(*(uint16_t*)&value[PZ003_RIR_ENERGY_L*2])  | __builtin_bswap16(*(uint16_t*)&value[PZ003_RIR_ENERGY_H*2]   << 16);
        alarmh  = __builtin_bswap16(*(uint16_t*)&value[PZ003_RIR_ALARM_H*2]);
        alarml  = __builtin_bswap16(*(uint16_t*)&value[PZ003_RIR_ALARM_L*2]);
        return true;
    }
};

/**
 * @brief a structure that reflects PZEM state/data values
 * 
 */
struct pzem_state {
    uint8_t addr = ADDR_ANY;
    metrics data;
    uint16_t alrmh_thrsh=0;
    uint16_t alrml_thrsh=0;
    bool alarmh=false;
    bool alarml=false;
    uint8_t irange = 0;     // 100A shunt
    pzmbus::pzem_err_t err;
    int64_t poll_us=0;     // last poll request sent time, microseconds since boot
    int64_t update_us=0;   // last succes update time, us since boot

    /**
     * @brief return age time since last update in ms
     * 
     * @return int64_t age time in ms
     */
    int64_t dataAge() const { return (esp_timer_get_time() - update_us)/1000; }

    /**
     * @brief update poll_us to current value
     * should be called on each request set to PZEM
     * 
     */
    void reset_poll_us(){ poll_us=esp_timer_get_time(); }

    /**
     * @brief data considered stale if last update time is more than PZEM_REFRESH_PERIOD
     * 
     * @return true if stale
     * @return false if data is fresh and valid
     */
    bool dataStale() const {return (esp_timer_get_time() - update_us > PZEM_REFRESH_PERIOD );}

    /**
     * @brief try to parse PZEM reply packet and update structure state
     * 
     * @param m RX message struct
     * @param skiponbad try to parse even packet has wrong MODBUS add or has bad CRC
     * @return true on success
     * @return false on error
     */
    bool parse_rx_mgs(const RX_msg *m, bool skiponbad=true){
        if (!m->valid && skiponbad)          // check if message is valid before parsing it further
            return false;

        if (m->addr != addr && skiponbad)    // this is not "my" packet
            return false;

        switch (static_cast<pzmbus::pzemcmd_t>(m->cmd)){
            case pzmbus::pzemcmd_t::RIR : {
                if(data.parse_rx_msg(m))  // try to parse it as a full metrics packet
                    break;
                else {
                    err = pzmbus::pzem_err_t::err_parse;
                    return false;
                }
            }
            case pzmbus::pzemcmd_t::RHR : {
                if (m->rawdata[2] == PZ003_RHR_CNT * 2){ // we got full len RHR data
                    alrmh_thrsh = __builtin_bswap16(*(uint16_t*)&m->rawdata[3]);
                    alrml_thrsh = __builtin_bswap16(*(uint16_t*)&m->rawdata[5]);
                    addr = m->rawdata[6];
                    irange = m->rawdata[8];
                }
                // unknown regs
                break;
            }
            case pzmbus::pzemcmd_t::WSR : {
                // 4th byte is reg ADDR_L
                switch (m->rawdata[3]){
                    case PZ003_RHR_ALARM_H :
                        alrmh_thrsh = __builtin_bswap16(*(uint16_t*)&m->rawdata[4]);
                        break;
                    case PZ003_RHR_ALARM_L :
                        alrml_thrsh = __builtin_bswap16(*(uint16_t*)&m->rawdata[4]);
                        break;
                    case PZ003_RHR_ADDR :
                        addr = m->rawdata[5];            // addr is only one byte
                        break;
                    case PZ003_RHR_CURRENT_RANGE :
                        irange = m->rawdata[5];          // shunt is only one byte
                        break;
                    default:
                        break;
                }
            }
            case pzmbus::pzemcmd_t::reset_energy :
                data.energy=0;                      // nothing to do, except reset conter
                break;
            case pzmbus::pzemcmd_t::read_err :
            case pzmbus::pzemcmd_t::write_err :
            case pzmbus::pzemcmd_t::reset_err :
            case pzmbus::pzemcmd_t::calibrate_err :
                // стоит ли здесь инвалидировать метрики???
                err = (pzmbus::pzem_err_t)m->rawdata[2];
                return true;
            default:
                break;
        }

        err = pzmbus::pzem_err_t::err_ok;
        update_us = esp_timer_get_time();
        return true;
    }

};

/**
 * @brief message request for all energy metrics
 * 
 * @param addr - slave device modbus address
 * @return TX_msg* 
 */
TX_msg* cmd_get_metrics(uint8_t addr = ADDR_ANY);

/**
 * @brief message request to get RHR values
 * there two regs - 'slave modbus addr' and 'alarm threshold', this will read both.
 * It is not possible to distinguish reply packets for one reg from another if reading only one reg
 * so we will read both and parse data output for required one
 * 
 * @param addr 
 * @return TX_msg* 
 */
TX_msg* cmd_get_opts(const uint8_t addr = ADDR_ANY);

TX_msg* cmd_set_modbus_addr(uint8_t new_addr, const uint8_t current_addr = ADDR_ANY);

/**
 * @brief message request to report current slave device modbus address
 * 
 * @param addr 
 * @return TX_msg* 
 */
TX_msg* cmd_get_modbus_addr(const uint8_t addr = ADDR_ANY);

/**
 * @brief message request to set new Power Alarm threshold value
 * 
 * @param w - watts threshold value
 * @param addr 
 * @return TX_msg* 
 */
TX_msg* cmd_set_alarmh_thr(uint16_t w, const uint8_t addr = ADDR_ANY);
TX_msg* cmd_set_alarml_thr(uint16_t w, const uint8_t addr = ADDR_ANY);

/**
 * @brief message request to get current Power Alarm threshold value
 * 
 * @param addr 
 * @return TX_msg* 
 */
TX_msg* cmd_get_alarm_thr(const uint8_t addr = ADDR_ANY);

TX_msg* cmd_energy_reset(const uint8_t addr = ADDR_ANY);

/**
 * @brief pretty print the content of RX packet data
 * 
 * @param m PZEM RX packet structure
 */
void rx_msg_prettyp(const RX_msg *m);
}