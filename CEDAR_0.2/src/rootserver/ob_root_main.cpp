/**
 * Copyright (C) 2013-2016 DaSE .
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * @file ob_root_main.cpp
 * @brief rootserver main, deal with the new system param for election
 *        store the cmd_rs_cluster_ips_ to the rs_config values.
 *
 * @version CEDAR 0.2 
 * @author Chu Jiajia  <52151500014@ecnu.cn>
 *         zhangcd <zhangcd_ecnu@ecnu.cn>
 * @date 2015_08_23
 */
/*===============================================================
*   (C) 2007-2010 Taobao Inc.
*
*
*   Version: 0.1 2010-09-26
*
*   Authors:
*          daoan(daoan@taobao.com)
*
*
================================================================*/
#include "rootserver/ob_root_main.h"
#include "common/ob_version.h"

namespace oceanbase
{
  using common::OB_SUCCESS;
  using common::OB_ERROR;
  namespace rootserver
  {
    ObRootMain::ObRootMain()
      : rs_reload_config_(rs_config_),
        config_mgr_(rs_config_, rs_reload_config_), worker(config_mgr_, rs_config_)
    {
    }

    common::BaseMain* ObRootMain::get_instance()
    {
      if (instance_ == NULL)
      {
        instance_ = new (std::nothrow)ObRootMain();
      }
      return instance_;
    }

    void ObRootMain::print_version()
    {
      fprintf(stderr, "rootserver (%s %s)\n", PACKAGE_STRING, RELEASEID);
      fprintf(stderr, "SVN_VERSION: %s\n", svn_version());
      fprintf(stderr, "BUILD_TIME: %s %s\n", build_date(), build_time());
      fprintf(stderr, "BUILD_FLAGS: %s\n\n", build_flags());
      fprintf(stderr, "Copyright (c) 2007-2013 Taobao Inc.\n");
    }

    static const int START_REPORT_SIG = 49;
    static const int START_MERGE_SIG = 50;
    static const int DUMP_ROOT_TABLE_TO_LOG = 51;
    static const int DUMP_AVAILABLE_SEVER_TO_LOG = 52;
    static const int SWITCH_SCHEMA = 53;
    static const int RELOAD_CONFIG = 54;
    static const int DO_CHECK_POINT = 55;
    static const int DROP_CURRENT_MERGE = 56;
    static const int CREATE_NEW_TABLE = 57;

    int ObRootMain::do_work()
    {
      int ret = OB_SUCCESS;
      char dump_config_path[OB_MAX_FILE_NAME_LENGTH];

      TBSYS_LOG(INFO, "oceanbase-root start svn_version=[%s] "
                "build_date=[%s] build_time=[%s]", svn_version(), build_date(),
                build_time());

      rs_reload_config_.set_root_server(worker.get_root_server());

      /* read config from binary config file if it existed. */
      snprintf(dump_config_path,
               sizeof (dump_config_path), "etc/%s.config.bin", server_name_);
      config_mgr_.set_dump_path(dump_config_path);
      if (OB_SUCCESS != (ret = config_mgr_.base_init()))
      {
        TBSYS_LOG(ERROR, "init config manager error, ret: [%d]", ret);
      }
      else if (OB_SUCCESS != (ret = config_mgr_.load_config(config_)))
      {
        TBSYS_LOG(ERROR, "load config error, path: [%s], ret: [%d]",
                  config_, ret);
      }
      else
      {
        TBSYS_LOG(INFO, "load config file successfully. path: [%s]",
                  strlen(config_) > 0 ? config_ : dump_config_path);
      }

      /* set configuration pass from command line */
      if (0 != cmd_cluster_id_)
      {
          rs_config_.cluster_id = static_cast<int64_t>(cmd_cluster_id_);
      }
      if (strlen(cmd_rs_ip_) > 0)
      {
        rs_config_.root_server_ip.set_value(cmd_rs_ip_); /* rs vip */
      }
      if (cmd_rs_port_ > 0)
      {
        rs_config_.port = cmd_rs_port_; /* listen port */
      }
      if (strlen(cmd_master_rs_ip_) > 0)
      {
        rs_config_.master_root_server_ip.set_value(cmd_master_rs_ip_);
      }
      if (cmd_master_rs_port_ > 0)
      {
        rs_config_.master_root_server_port = cmd_master_rs_port_;
      }
      if (strlen(cmd_devname_) > 0)
      {
        rs_config_.devname.set_value(cmd_devname_);
      }
      if (strlen(cmd_extra_config_) > 0
          && OB_SUCCESS != (ret = rs_config_.add_extra_config(cmd_extra_config_)))
      {
        TBSYS_LOG(ERROR, "Parse extra config error! string: [%s], ret: [%d]",
                  cmd_extra_config_, ret);
      }
      // add by zcd [multi_cluster] 20150416:b
      // 判断命令行的-s参数输入是否有效
      if(strlen(cmd_rs_cluster_ips_) > 0)
      {
        rs_config_.all_cluster_rs_ip.set_value(cmd_rs_cluster_ips_);
      }
      // add:e

      //add chujiajia [rs_election][multi_cluster] 20150929:b
      if (cmd_rs_election_random_wait_time_ > 0)
      {
        rs_config_.rs_election_random_wait_time = cmd_rs_election_random_wait_time_; /* listen port */
      }
      // add:e
      rs_config_.print();

      if (OB_SUCCESS != ret)
      {
      }
      else if (OB_SUCCESS != (ret = rs_config_.check_all()))
      {
        TBSYS_LOG(ERROR, "check config failed, ret: [%d]", ret);
      }
      else
      {
        // add signal I want to catch
        // we don't process the following
        // signals any more, but receive them for backward
        // compatibility
        add_signal_catched(START_REPORT_SIG);
        add_signal_catched(START_MERGE_SIG);
        add_signal_catched(DUMP_ROOT_TABLE_TO_LOG);
        add_signal_catched(DUMP_AVAILABLE_SEVER_TO_LOG);
        add_signal_catched(SWITCH_SCHEMA);
        add_signal_catched(RELOAD_CONFIG);
        add_signal_catched(DO_CHECK_POINT);
        add_signal_catched(DROP_CURRENT_MERGE);
        add_signal_catched(CREATE_NEW_TABLE);
      }

      if (OB_SUCCESS != ret)
      {
        TBSYS_LOG(ERROR, "start root server failed, ret: [%d]", ret);
      }
      else
      {
        worker.set_io_thread_count((int32_t)rs_config_.io_thread_count);
        ret = worker.start(false);
      }
      return ret;
    }

    void ObRootMain::do_signal(const int sig)
    {
      switch(sig)
      {
        case SIGTERM:
        case SIGINT:
          signal(SIGINT, SIG_IGN);
          signal(SIGTERM, SIG_IGN);
          TBSYS_LOG(INFO, "stop signal received");
          worker.stop();
          break;
        default:
          TBSYS_LOG(INFO, "signal processed by base_main, sig=%d", sig);
          break;
      }
    }
  }
}
