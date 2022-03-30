/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: Leo Suardi <lsuardi@student.42.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/29 22:13:35 by Leo Suardi        #+#    #+#             */
/*   Updated: 2022/03/31 01:16:29 by Leo Suardi       ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

/*-------------- UTILS --------------*/

static bool		is_pipeline( int ac, char **av )
{
	while (ac-- && strcmp(*av, ";"))
	{
		if (!strcmp(*av++, "|"))
			return (true);
	}
	return (false);
}

static size_t	ft_strlen( const char *s )
{
	const char	*ptr = s;

	while (*ptr)
		++ptr;
	return (ptr - s);
}

static void		*ft_memcpy( void *lhs, const void *rhs, size_t n )
{
	char		*dst = lhs;
	const char	*src = rhs;

	while (n--)
		*dst++ = *src++;
	return (lhs);
}

/*-----------------------------------*/





/*-------------- ERROR --------------*/

static void	error_exec( const char *exec_name )
{
	write(2, "error: cannot execute ", 22);
	write(2, exec_name, ft_strlen(exec_name));
	write(2, "\n", 1);
	exit(EXIT_FAILURE);
}

static void	error_fatal( void )
{
	write(2, "error: fatal\n", 13);
	exit(EXIT_FAILURE);
}

/*-----------------------------------*/





/*---------------- CD ---------------*/

static int	builtin_cd( int argc, char **argv )
{
	if (argc != 2)
	{
		write(2, "error: cd: bad arguments\n", 25);
		return (1);
	}
	if (chdir(argv[1]))
	{
		write(2, "error: cd: cannot change directory to ", 38);
		write(2, argv[1], ft_strlen(argv[1]));
		return (1);
	}
	return (0);
}

/*-----------------------------------*/





/*------------- EXECUTOR ------------*/

static int	get_exec_ac( int ac, char **av )
{
	int	exec_ac = 0;

	while (exec_ac < ac && strcmp(av[exec_ac], ";") && strcmp(av[exec_ac], "|"))
		++exec_ac;
	return (exec_ac);
}

static int	execute( int ac, char **av, char **ep, int in, int out, int next_in )
{
	pid_t	cpid;
	int		exec_ac = get_exec_ac(ac, av);
	char	*exec_av[exec_ac + 1];

	exec_av[exec_ac] = 0;
	ft_memcpy(exec_av, av, exec_ac * sizeof(char*));
	if (!strcmp(*av, "cd"))
		builtin_cd(exec_ac, exec_av);
	else
	{
		cpid = fork();
		if (cpid == -1)
			error_fatal();
		if (!cpid)
		{
			if (in != 0)
			{
				dup2(in, 0);
				close(in);
			}
			if (out != 1)
			{
				dup2(out, 1);
				close(out);
			}
			close(next_in);
			execve(*exec_av, exec_av, ep);
			error_exec(*exec_av);
		}
	}
	if (in != 0)
		close(in);
	if (out != 1)
		close(out);
	return (cpid);
}

static int	execute_pipeline( int ac, char **av, char **ep )
{
	int		pfd[2];
	int		in, out;
	int		offset;
	pid_t	cpid;

	offset = 0;
	in = 0;
	cpid = fork();
	if (!cpid)
	{
		while (offset < ac && strcmp(av[offset], ";"))
		{
			pipe(pfd);
			if (is_pipeline(ac - offset, av + offset))
				out = pfd[1];
			else
				out = 1;
			execute(ac - offset, av + offset, ep, in, out, pfd[0]);
			close(pfd[1]);
			if (in != 0)
				close(in);
			in = pfd[0];
			while (offset < ac
					&& strcmp(av[offset], ";")
					&& strcmp(av[offset++], "|"))
				continue ;
		}
		close(pfd[0]);
		while (wait(NULL) != -1)
			continue ;
		exit(0);
	}
	while (offset < ac && strcmp(av[offset], ";"))
		++offset;
	wait(NULL);
	return (offset);
}

/*-----------------------------------*/





int	main( int argc, char **argv, char **envp )
{
	int	i;

	i = 1;
	while (i < argc)
	{
		while (!strcmp(argv[i], ";"))
			if (++i == argc)
				goto end ;
		if (is_pipeline(argc - i, argv + i))
			i += execute_pipeline(argc - i, argv + i, envp);
		else
			i += execute(argc - i, argv + i, envp, 0, 1, 0);
	}
	end: return (0);
}
