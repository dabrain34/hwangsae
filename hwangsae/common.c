/**
 *  Copyright 2019 SK Telecom Co., Ltd.
 *    Author: Walter Lozano <walter.lozano@collabora.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "common.h"

#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <gst/gstclock.h>
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

gchar *
hwangsae_common_get_local_ip (void)
{
  struct ifaddrs *addrs;
  struct ifaddrs *it;
  gchar *result = NULL;

  if (getifaddrs (&addrs) < 0) {
    return NULL;
  }

  for (it = addrs; !result && it; it = it->ifa_next) {
    socklen_t addr_len;
    char buf[INET6_ADDRSTRLEN + 1];

    /* Ignore interfaces that are down, not running or don't have an IP. We also
     * want to skip loopbacks. */
    if ((it->ifa_flags & (IFF_UP | IFF_RUNNING | IFF_LOOPBACK)) !=
        (IFF_UP | IFF_RUNNING) || it->ifa_addr == NULL) {
      continue;
    }

    switch (it->ifa_addr->sa_family) {
      case AF_INET:
        addr_len = sizeof (struct sockaddr_in);
        break;
      case AF_INET6:
        addr_len = sizeof (struct sockaddr_in6);
        break;
      default:
        continue;
    }

    if (getnameinfo (it->ifa_addr, addr_len, buf, sizeof (buf), NULL, 0,
            NI_NUMERICHOST) != 0) {
      continue;
    }

    result = g_strdup (buf);
  }

  freeifaddrs (addrs);

  return result;
}

GSettings *
hwangsae_common_gsettings_new (const gchar * schema_id)
{
  g_autoptr (GSettingsBackend) backend = NULL;

  backend = g_keyfile_settings_backend_new ("/etc/hwangsaeul.conf", "/", NULL);

  return g_settings_new_with_backend (schema_id, backend);
}

gboolean
hwangsae_common_parse_times_from_filename (const gchar * filename,
    guint64 * start_time, guint64 * end_time)
{
  gboolean result = FALSE;
  gchar **parts = g_strsplit_set (filename, "-.", -1);
  gchar *endptr;
  guint len = g_strv_length (parts);
  guint64 start;
  guint64 end;

  if (len < 4) {
    goto out;
  }

  /*
   * /path/to/file/recording-name-starttimeusec-endtimeusec.ts
   */

  start = g_ascii_strtoull (parts[len - 3], &endptr, 10) * GST_USECOND;
  if (*endptr != '\0') {
    goto out;
  }

  end = g_ascii_strtoull (parts[len - 2], &endptr, 10) * GST_USECOND;
  if (*endptr != '\0') {
    goto out;
  }

  if (start_time) {
    *start_time = start;
  }
  if (end_time) {
    *end_time = end;
  }

  result = TRUE;

out:
  g_strfreev (parts);

  return result;
}
