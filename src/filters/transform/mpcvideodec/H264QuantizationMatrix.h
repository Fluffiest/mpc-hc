/* 
 * $Id: H264QuantizationMatrix.h 249 2007-09-26 11:07:22Z casimir666 $
 *
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

typedef enum
{
	JVTMatrix,
	QMatrix,
	QMatrix2,
	Flat16
} QMatrixH264Type;

const DXVA_Qmatrix_H264		g_QMatrixH264[] = 
{
	// 1) The JVT Matrix
	{
		{
			// INTRA4X4_LUMA
			{	6,13,20,28,		
				13,20,28,32,
				20,28,32,37,
				28,32,37,42 },

			// INTRA4X4_CHROMAU
			{	6,13,20,28,
				13,20,28,32,
				20,28,32,37,
				28,32,37,42 },

			// INTRA4X4_CHROMAV
			{	6,13,20,28,
				13,20,28,32,
				20,28,32,37,
				28,32,37,42 },

			// INTER4X4_LUMA
			{	10,14,20,24,
				14,20,24,27,
				20,24,27,30,
				24,27,30,34 },

			// INTER4X4_CHROMAU
			{	10,14,20,24,
				14,20,24,27,
				20,24,27,30,
				24,27,30,34 },

			// INTER4X4_CHROMAV
			{	10,14,20,24,
				14,20,24,27,
				20,24,27,30,
				24,27,30,34 } 
		},
		{
			// INTRA8X8_LUMA
			{	6,10,13,16,18,23,25,27,
				10,11,16,18,23,25,27,29,
				13,16,18,23,25,27,29,31,
				16,18,23,25,27,29,31,33,
				18,23,25,27,29,31,33,36,
				23,25,27,29,31,33,36,38,
				25,27,29,31,33,36,38,40,
				27,29,31,33,36,38,40,42 },

			// INTER8X8_LUMA
			{	9,13,15,17,19,21,22,24,
				13,13,17,19,21,22,24,25,
				15,17,19,21,22,24,25,27,
				17,19,21,22,24,25,27,28,
				19,21,22,24,25,27,28,30,
				21,22,24,25,27,28,30,32,
				22,24,25,27,28,30,32,33,
				24,25,27,28,30,32,33,35 }
		}
	},

	// 2) q_matrix
	{
		{
			// INTRA4X4_LUMA =
			{	6,12,19,26,
				12,19,26,31,
				19,26,31,35,
				26,31,35,39 },

			// INTRA4X4_CHROMAU =
			{	6,12,19,26,
				12,19,26,31,
				19,26,31,35,
				26,31,35,39 },

			// INTRA4X4_CHROMAV =
			{	6,12,19,26,
				12,19,26,31,
				19,26,31,35,
				26,31,35,39 },

			// INTER4X4_LUMA =
			{	9,13,18,21,
				13,18,21,24,
				18,21,24,27,
				21,24,27,30 },

			// INTER4X4_CHROMAU =
			{	9,13,18,21,
				13,18,21,24,
				18,21,24,27,
				21,24,27,30 },

			// INTER4X4_CHROMAV =
			{	9,13,18,21,
				13,18,21,24,
				18,21,24,27,
				21,24,27,30 }
		},
		{
			// INTRA8X8_LUMA =
			{	6,10,13,16,19,24,26,28,
				10,12,16,19,24,26,28,31,
				13,16,19,24,26,28,31,33,
				16,19,24,26,28,31,33,35,
				19,24,26,28,31,33,35,37,
				24,26,28,31,33,35,37,39,
				26,28,31,33,35,37,39,42,
				28,31,33,35,37,39,42,44 },

			// INTER8X8_LUMA =
			{	9,12,14,16,18,19,21,22,
				12,13,16,18,19,21,22,24,
				14,16,18,19,21,22,24,25,
				16,18,19,21,22,24,25,27,
				18,19,21,22,24,25,27,28,
				19,21,22,24,25,27,28,30,
				21,22,24,25,27,28,30,31,
				22,24,25,27,28,30,31,33 }
		}
	},

	// 3) q_matrix2
	{
		{
			// INTRA4X4_LUMA =
			{ 7,16,22,24,
			16,22,24,28,
			18,22,27,33,
			22,24,32,47 },

			// INTRA4X4_CHROMAU =
			{ 7,16,22,24,
			16,22,24,28,
			18,22,27,33,
			22,24,32,47 },

			// INTRA4X4_CHROMAV =
			{ 7,16,22,24,
			16,22,24,28,
			18,22,27,33,
			22,24,32,47 },

			// INTER4X4_LUMA =
			{13,15,17,18,
			15,17,18,20,
			17,18,21,22,
			18,20,22,25 },

			// INTER4X4_CHROMAU =
			{13,15,17,18,
			15,17,18,20,
			17,18,21,22,
			18,20,22,25 },

			// INTER4X4_CHROMAV =
			{13,15,17,18,
			15,17,18,20,
			17,18,21,22,
			18,20,22,25 }
		},
		{
			// INTRA8X8_LUMA =
			{ 7,13,16,18,22,22,24,28,
			13,13,18,20,22,24,28,31,
			16,18,22,22,24,28,28,32,
			18,18,22,22,24,28,31,33,
			18,22,22,24,27,29,33,40,
			22,22,24,27,29,33,40,48,
			22,22,24,28,32,38,47,57,
			22,24,29,32,38,47,57,69 },

			// INTER8X8_LUMA =
			{ 13,14,15,16,17,17,18,19,
			14,15,16,17,17,18,19,20,
			15,16,17,17,18,19,20,21,
			16,17,17,18,19,20,21,22,
			17,17,18,19,21,22,22,23,
			17,18,19,20,22,22,23,25,
			18,19,20,22,22,23,25,26,
			19,20,21,22,23,25,26,27 }
		}
	},

	// 4) #flat 16
	{
		{
			// INTRA4X4_LUMA =
			{ 16,16,16,16,
			16,16,16,16,
			16,16,16,16,
			16,16,16,16 },

			// INTRA4X4_CHROMAU =
			{ 16,16,16,16,
			16,16,16,16,
			16,16,16,16,
			16,16,16,16 },

			// INTRA4X4_CHROMAV =
			{ 16,16,16,16,
			16,16,16,16,
			16,16,16,16,
			16,16,16,16 },

			// INTER4X4_LUMA =
			{ 16,16,16,16,
			16,16,16,16,
			16,16,16,16,
			16,16,16,16 },

			// INTER4X4_CHROMAU =
			{ 16,16,16,16,
			16,16,16,16,
			16,16,16,16,
			16,16,16,16 },

			// INTER4X4_CHROMAV =
			{ 16,16,16,16,
			16,16,16,16,
			16,16,16,16,
			16,16,16,16 }
		},
		{
			// INTRA8X8_LUMA =
			{ 16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16 },

			// INTER8X8_LUMA =
			{ 16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16,
			16,16,16,16,16,16,16,16 }
		}
	}
};