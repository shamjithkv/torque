#include "license_pbs.h" /* See here for the software license */
/*
 *
 * qsig - (PBS) signal a batch job
 *
 * Authors:
 *      Terry Heidelberg
 *      Livermore Computing
 *
 *      Bruce Kelly
 *      National Energy Research Supercomputer Center
 *
 *      Lawrence Livermore National Laboratory
 *      University of California
 */

#include "cmds.h"
#include <pbs_config.h>   /* the master config generated by configure */
#include "../lib/Libifl/lib_ifl.h"


int main(

  int    argc,
  char **argv) /* qsig */

  {
  int c;
  int errflg = 0;
  int any_failed = 0;
  int runAsync = FALSE;

  char job_id[PBS_MAXCLTJOBID];       /* from the command line */

  char job_id_out[PBS_MAXCLTJOBID];
  char server_out[MAXSERVERNAME] = "";
  char rmt_server[MAXSERVERNAME];

#define MAX_SIGNAL_TYPE_LEN 32
  static char sig_string[MAX_SIGNAL_TYPE_LEN+1] = "SIGTERM";

#define GETOPT_ARGS "as:"

  while ((c = getopt(argc, argv, GETOPT_ARGS)) != EOF)
    switch (c)
      {

      case 'a':

        runAsync = TRUE;

        break;

      case 's':

        snprintf(sig_string, sizeof(sig_string), "%s", optarg);

        break;

      default :
        errflg++;
      }

  if (errflg || optind >= argc)
    {
    static char usage[] = "usage: qsig [-a] [-s signal] job_identifier...\n";
    fprintf(stderr,"%s", usage);
    exit(2);
    }

  for (; optind < argc; optind++)
    {
    int connect;
    int stat = 0;
    int located = FALSE;

    snprintf(job_id, sizeof(job_id), "%s", argv[optind]);

    if (get_server(job_id, job_id_out, sizeof(job_id_out), server_out, sizeof(server_out)))
      {
      fprintf(stderr, "qsig: illegally formed job identifier: %s\n", job_id);
      any_failed = 1;
      continue;
      }

cnt:

    connect = cnt2server(server_out);

    if (connect <= 0)
      {
      any_failed = -1 * connect;

      if (server_out[0] != 0)
        fprintf(stderr, "qsig: cannot connect to server %s (errno=%d) %s\n",
              server_out, any_failed, pbs_strerror(any_failed));
      else
        fprintf(stderr, "qsig: cannot connect to server %s (errno=%d) %s\n",
              pbs_server, any_failed, pbs_strerror(any_failed));
      
      continue;
      }

    if (runAsync == TRUE)
      {
      stat = pbs_sigjobasync_err(connect,job_id_out,sig_string,NULL, &any_failed);
      }
    else
      {
      stat = pbs_sigjob_err(connect, job_id_out, sig_string, NULL, &any_failed);
      }

    if (stat && (any_failed != PBSE_UNKJOBID))
      {
      prt_job_err("qsig", connect, job_id_out);
      }
    else if (stat && (any_failed == PBSE_UNKJOBID) && !located)
      {
      located = TRUE;

      if (locate_job(job_id_out, server_out, rmt_server))
        {
        pbs_disconnect(connect);
        strcpy(server_out, rmt_server);
        goto cnt;
        }

      prt_job_err("qsig", connect, job_id_out);
      }

    pbs_disconnect(connect);
    }

  exit(any_failed);
  }
